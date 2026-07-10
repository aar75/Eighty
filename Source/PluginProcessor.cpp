#include "PluginProcessor.h"
#include "PluginEditor.h"

EightyProcessor::EightyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", eighty::createParameterLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            raw[rp->paramID] = apvts.getRawParameterValue (rp->paramID);

    // engine note stream -> hosted synth layer
    engine.noteEcho = [this] (int note, float vel, bool on, int offset)
    {
        midiEcho.addEvent (on ? juce::MidiMessage::noteOn (1, note, vel)
                              : juce::MidiMessage::noteOff (1, note),
                           juce::jmax (0, offset));
    };

    formatManager.addDefaultFormats();   // VST3 hosting (JUCE_PLUGINHOST_VST3)
    if (auto xml = juce::parseXML (pluginCacheFile()))
        knownPlugins.recreateFromXml (*xml);

    // If a previous scan died mid-plugin, the dead-man's-pedal file still
    // names the culprit - blacklist it so the next scan skips it.
    juce::PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal (
        knownPlugins, deadMansPedalFile());
}

// ------------------------------------------------ external insert chain
juce::File EightyProcessor::pluginCacheFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Eighty").getChildFile ("pluginCache.xml");
}

juce::File EightyProcessor::deadMansPedalFile() const
{
    return pluginCacheFile().getSiblingFile ("scanInProgress.txt");
}

void EightyProcessor::saveKnownPlugins()
{
    if (auto xml = knownPlugins.createXml())   // includes the blacklist
    {
        pluginCacheFile().getParentDirectory().createDirectory();
        xml->writeTo (pluginCacheFile());
    }
}

void EightyProcessor::rescanPlugins()
{
    // Crash-safe scan: the dead-man's-pedal file names the plugin being
    // probed, so if that plugin kills the process it gets blacklisted on
    // the next launch instead of crashing us forever. The cache is saved
    // incrementally so a crash loses at most a few entries, and already-
    // scanned plugins are skipped when the user rescans.
    juce::PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal (
        knownPlugins, deadMansPedalFile());

    juce::VST3PluginFormat format;
    juce::PluginDirectoryScanner scanner (knownPlugins, format,
                                          format.getDefaultLocationsToSearch(),
                                          true, deadMansPedalFile());
    juce::String progressName;
    int sinceSave = 0;
    bool more = true;
    while (more)
    {
        more = scanner.scanNextFile (true, progressName);
        if (++sinceSave >= 4)
        {
            sinceSave = 0;
            saveKnownPlugins();
        }
    }
    saveKnownPlugins();
}

bool EightyProcessor::addInsert (const juce::PluginDescription& desc, juce::String& error)
{
    if (getNumInserts() >= kMaxInserts)
    {
        error = "Insert chain is full";
        return false;
    }
    auto instance = formatManager.createPluginInstance (desc, curSampleRate, curBlockSize, error);
    if (instance == nullptr)
        return false;

    instance->setPlayConfigDetails (2, 2, curSampleRate, curBlockSize);
    instance->prepareToPlay (curSampleRate, curBlockSize);

    {
        const juce::ScopedLock sl (chainLock);
        insertChain.add (instance.release());
    }
    updateChainLatency();
    ++chainVersion;
    return true;
}

void EightyProcessor::removeInsert (int index)
{
    std::unique_ptr<juce::AudioPluginInstance> removed;
    {
        const juce::ScopedLock sl (chainLock);
        if (index < 0 || index >= insertChain.size()) return;
        removed.reset (insertChain.removeAndReturn (index));
    }
    removed->releaseResources();
    removed.reset();                 // destroy outside the audio lock
    updateChainLatency();
    ++chainVersion;
}

int EightyProcessor::getNumInserts() const
{
    const juce::ScopedLock sl (chainLock);
    return insertChain.size();
}

juce::String EightyProcessor::getInsertName (int index) const
{
    const juce::ScopedLock sl (chainLock);
    return index >= 0 && index < insertChain.size() ? insertChain[index]->getName()
                                                    : juce::String();
}

juce::AudioPluginInstance* EightyProcessor::getInsert (int index) const
{
    const juce::ScopedLock sl (chainLock);
    return index >= 0 && index < insertChain.size() ? insertChain[index] : nullptr;
}

void EightyProcessor::updateChainLatency()
{
    int latency = 0;
    {
        const juce::ScopedLock sl (chainLock);
        for (auto* p : insertChain)
            latency += p->getLatencySamples();
    }
    setLatencySamples (latency);
}

static juce::ValueTree pluginInstanceToTree (juce::AudioPluginInstance& p)
{
    juce::ValueTree e ("plugin");
    if (auto descXml = p.getPluginDescription().createXml())
        e.setProperty ("desc", descXml->toString(), nullptr);
    juce::MemoryBlock mb;
    p.getStateInformation (mb);
    e.setProperty ("state", mb.toBase64Encoding(), nullptr);
    return e;
}

static bool descFromTree (const juce::ValueTree& e, juce::PluginDescription& desc)
{
    if (auto descXml = juce::parseXML (e.getProperty ("desc").toString()))
        return desc.loadFromXml (*descXml);
    return false;
}

static void applyStateFromTree (const juce::ValueTree& e, juce::AudioPluginInstance* inst)
{
    juce::MemoryBlock mb;
    if (inst != nullptr && mb.fromBase64Encoding (e.getProperty ("state").toString()))
        inst->setStateInformation (mb.getData(), (int) mb.getSize());
}

juce::ValueTree EightyProcessor::chainToValueTree() const
{
    juce::ValueTree t ("insertChain");
    const juce::ScopedLock sl (chainLock);
    for (auto* p : insertChain)
        t.appendChild (pluginInstanceToTree (*p), nullptr);
    return t;
}

void EightyProcessor::restoreChainFromValueTree (juce::ValueTree chain, juce::ValueTree synth)
{
    while (getNumInserts() > 0)
        removeInsert (0);
    clearSynthLayer();

    for (auto e : chain)
    {
        if (! e.hasType ("plugin")) continue;
        juce::PluginDescription desc;
        if (! descFromTree (e, desc)) continue;
        juce::String error;
        if (addInsert (desc, error))
            applyStateFromTree (e, getInsert (getNumInserts() - 1));
    }

    auto e = synth.getChildWithName ("plugin");
    if (e.isValid())
    {
        juce::PluginDescription desc;
        juce::String error;
        if (descFromTree (e, desc) && setSynthLayer (desc, error))
            applyStateFromTree (e, getSynthLayer());
    }
}

void EightyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    curSampleRate = sampleRate;
    curBlockSize = samplesPerBlock;
    engine.prepare (sampleRate, juce::jmax (samplesPerBlock, 32));
    chorus.prepare (sampleRate, samplesPerBlock);
    delay.prepare (sampleRate);
    tremolo.prepare (sampleRate);
    scratch.setSize (1, juce::jmax (samplesPerBlock, 32));

    synthBuf.setSize (2, juce::jmax (samplesPerBlock, 32));

    {
        const juce::ScopedLock sl (chainLock);
        for (auto* p : insertChain)
        {
            p->releaseResources();
            p->setPlayConfigDetails (2, 2, sampleRate, samplesPerBlock);
            p->prepareToPlay (sampleRate, samplesPerBlock);
        }
        if (synthLayer != nullptr)
        {
            synthLayer->releaseResources();
            synthLayer->setPlayConfigDetails (0, 2, sampleRate, samplesPerBlock);
            synthLayer->prepareToPlay (sampleRate, samplesPerBlock);
        }
    }
    updateChainLatency();
}

// ------------------------------------------------- hosted synth layer
bool EightyProcessor::setSynthLayer (const juce::PluginDescription& desc, juce::String& error)
{
    auto inst = formatManager.createPluginInstance (desc, curSampleRate, curBlockSize, error);
    if (inst == nullptr)
        return false;

    inst->enableAllBuses();
    inst->setPlayConfigDetails (0, 2, curSampleRate, curBlockSize);
    inst->prepareToPlay (curSampleRate, curBlockSize);

    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (chainLock);
        old = std::move (synthLayer);
        synthLayer = std::move (inst);
    }
    old.reset();
    ++chainVersion;
    return true;
}

void EightyProcessor::clearSynthLayer()
{
    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (chainLock);
        old = std::move (synthLayer);
    }
    if (old != nullptr)
    {
        old->releaseResources();
        old.reset();
        ++chainVersion;
    }
}

juce::String EightyProcessor::getSynthLayerName() const
{
    const juce::ScopedLock sl (chainLock);
    return synthLayer != nullptr ? synthLayer->getName() : juce::String();
}

juce::AudioPluginInstance* EightyProcessor::getSynthLayer() const
{
    const juce::ScopedLock sl (chainLock);
    return synthLayer.get();
}

bool EightyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void EightyProcessor::updateParameters()
{
    using namespace eighty;
    auto& vp = engine.vp;
    auto& es = engine.es;

    for (int c = 0; c < 2; ++c)
    {
        auto& o = vp.osc[c];
        const bool two = c == 1;
        o.on        = rawVal (two ? ID::osc2On : ID::osc1On) > 0.5f;
        const int foot = (int) rawVal (two ? ID::osc2Foot : ID::osc1Foot);
        o.footSemis = (float) ((foot - 2) * 12); // 32'/16'/8'/4' -> -24/-12/0/+12
        o.semi      = two ? rawVal (ID::osc2Semi) : 0.f;
        o.fine      = rawVal (two ? ID::osc2Fine : ID::osc1Fine);
        o.saw       = rawVal (two ? ID::osc2Saw : ID::osc1Saw);
        o.pulse     = rawVal (two ? ID::osc2Pulse : ID::osc1Pulse);
        o.pw        = rawVal (two ? ID::osc2PW : ID::osc1PW);
        o.pwmDepth  = rawVal (two ? ID::osc2PWM : ID::osc1PWM);
        o.level     = rawVal (two ? ID::osc2Level : ID::osc1Level);
    }
    vp.noiseLevel   = rawVal (ID::noiseLevel);

    vp.hpfCutoff    = rawVal (ID::hpfCutoff);
    vp.lpfCutoff    = rawVal (ID::lpfCutoff);
    vp.resonance    = rawVal (ID::resonance);
    vp.filterEnvAmt = rawVal (ID::filterEnvAmt);
    vp.keyTrack     = rawVal (ID::keyTrack);
    vp.filterDrive  = rawVal (ID::filterDrive);

    vp.fA = rawVal (ID::fEnvA); vp.fD = rawVal (ID::fEnvD);
    vp.fS = rawVal (ID::fEnvS); vp.fR = rawVal (ID::fEnvR);
    vp.aA = rawVal (ID::aEnvA); vp.aD = rawVal (ID::aEnvD);
    vp.aS = rawVal (ID::aEnvS); vp.aR = rawVal (ID::aEnvR);
    vp.velToAmp    = rawVal (ID::velToAmp);
    vp.velToFilter = rawVal (ID::velToFilter);

    vp.lfoToPitch  = rawVal (ID::lfoToPitch);
    vp.lfoToFilter = rawVal (ID::lfoToFilter);
    vp.lfoToAmp    = rawVal (ID::lfoToAmp);

    vp.touchRise     = rawVal (ID::touchRise);
    vp.touchToVib    = rawVal (ID::touchToVib);
    vp.touchToBright = rawVal (ID::touchToBright);
    vp.touchToLevel  = rawVal (ID::touchToLevel);

    // Jupiter-8 card
    auto& jp = vp.jp;
    jp.vco1Wave      = (int) rawVal (ID::jpVco1Wave);
    jp.vco1FootSemis = (float) (((int) rawVal (ID::jpVco1Range) - 1) * 12); // 16'/8'/4'/2'
    jp.pw            = rawVal (ID::jpPW);
    jp.pwmDepth      = rawVal (ID::jpPWM);
    jp.mix           = rawVal (ID::jpMix);
    jp.vco2Wave      = (int) rawVal (ID::jpVco2Wave);
    jp.vco2FootSemis = (float) (((int) rawVal (ID::jpVco2Range) - 1) * 12);
    jp.semi          = rawVal (ID::jpVco2Semi);
    jp.fine          = rawVal (ID::jpVco2Fine);
    jp.sync          = rawVal (ID::jpSync) > 0.5f;
    jp.xmod          = rawVal (ID::jpXmod);
    jp.hpf           = rawVal (ID::jpHpf);
    jp.lpf           = rawVal (ID::jpLpf);
    jp.res           = rawVal (ID::jpRes);
    jp.slope24       = rawVal (ID::jpSlope24) > 0.5f;
    jp.envAmt        = rawVal (ID::jpEnvAmt);
    jp.keyTrack      = rawVal (ID::jpKeyTrk);
    jp.fA = rawVal (ID::jpFEnvA); jp.fD = rawVal (ID::jpFEnvD);
    jp.fS = rawVal (ID::jpFEnvS); jp.fR = rawVal (ID::jpFEnvR);
    jp.aA = rawVal (ID::jpAEnvA); jp.aD = rawVal (ID::jpAEnvD);
    jp.aS = rawVal (ID::jpAEnvS); jp.aR = rawVal (ID::jpAEnvR);

    es.engineMode = (int) rawVal (ID::engineMode);
    es.splitPoint = (int) rawVal (ID::splitPoint);
    es.csLow      = rawVal (ID::splitCsLow) > 0.5f;

    // CS/JP balance only applies when both engines can sound (Split/Layer);
    // center = both at full, edges mute one side
    const float bal = rawVal (ID::engineBalance);
    if (es.engineMode >= 2)
    {
        vp.csGain = juce::jmin (1.f, 2.f * (1.f - bal));
        vp.jpGain = juce::jmin (1.f, 2.f * bal);
    }
    else
        vp.csGain = vp.jpGain = 1.f;

    vp.drift        = rawVal (ID::drift);
    vp.stereoSpread = rawVal (ID::stereoSpread);
    vp.glideTime    = rawVal (ID::glideTime);
    vp.glideMode    = (int) rawVal (ID::glideMode);
    vp.masterTuneCents = rawVal (ID::masterTune);

    es.mode         = (SynthEngine::Mode) (int) rawVal (ID::voiceMode);
    es.polyVoices   = (int) rawVal (ID::polyVoices);
    es.unisonCount  = (int) rawVal (ID::unisonCount);
    es.unisonDetune = rawVal (ID::unisonDetune);
    es.bendRange    = (int) rawVal (ID::bendRange);

    es.arpMode    = (int) rawVal (ID::arpMode);
    es.arpSync    = rawVal (ID::arpSync) > 0.5f;
    es.arpRateHz  = rawVal (ID::arpRateHz);
    es.arpDiv     = (int) rawVal (ID::arpDiv);
    es.arpOctaves = (int) rawVal (ID::arpOctaves);
    es.arpGate    = rawVal (ID::arpGate);

    es.lfoRate  = rawVal (ID::lfoRate);
    es.lfoWave  = (int) rawVal (ID::lfoWave);
    es.lfoDelay = rawVal (ID::lfoDelay);
    es.pwmRate  = rawVal (ID::pwmRate);
    es.bpm      = hostBpm;

    // edge-detected toggles
    const bool arpOn = rawVal (ID::arpOn) > 0.5f;
    if (arpOn != lastArpOn)
    {
        es.arpOn = arpOn;
        engine.arpModeChanged (arpOn);
        lastArpOn = arpOn;
    }
    es.arpOn = arpOn;

    const bool holdOn = rawVal (ID::hold) > 0.5f;
    if (holdOn != lastHold)
    {
        engine.setHold (holdOn);
        lastHold = holdOn;
    }
}

void EightyProcessor::handleMidiEvent (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        engine.setPitchBend (m.getPitchWheelValue());
    else if (m.isChannelPressure())
        engine.setChannelPressure ((float) m.getChannelPressureValue() / 127.f);
    else if (m.isAftertouch())
        engine.setPolyPressure (m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.f);
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.allNotesOff();
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const float v = (float) m.getControllerValue() / 127.f;

        if (cc == 1)  { engine.setModWheel (v); return; }
        if (cc == 64) { engine.setSustainPedal (v > 0.5f); return; }

        if (midiLearn.handleCC (cc)) { lastLearnedCC.store (cc); return; }

        auto pid = midiLearn.paramForCC (cc);
        if (pid.isNotEmpty())
            if (auto* p = apvts.getParameter (pid))
                p->setValueNotifyingHost (v);
    }
}

void EightyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;

    updateParameters();

    // merge UI keyboard notes into the midi stream
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // UI pitch wheel (arrow keys / mouse) overrides only while active
    if (uiBendActive.load())
        engine.setBendNorm (uiBend.load());

    const int total = buffer.getNumSamples();
    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // temp right channel for mono outputs
    if (right == nullptr)
    {
        scratch.setSize (1, total, false, false, true);
        scratch.clear();
        right = scratch.getWritePointer (0);
    }

    // render segments split at midi event boundaries
    midiEcho.clear();
    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, total, (int) meta.samplePosition);
        if (t > pos)
        {
            engine.render (left + pos, right + pos, t - pos, pos);
            pos = t;
        }
        const auto msg = meta.getMessage();
        // expression data passes straight through to the synth layer
        if (msg.isPitchWheel() || msg.isController() || msg.isChannelPressure()
            || msg.isAftertouch())
            midiEcho.addEvent (msg, t);
        engine.echoBase = t;
        handleMidiEvent (msg);
    }
    if (pos < total)
        engine.render (left + pos, right + pos, total - pos, pos);
    midi.clear();

    // hosted VST3 synth layer: plays the engine's effective note stream
    // (post arp/hold), mixed in before the FX chain
    if (buffer.getNumChannels() >= 2)
    {
        const juce::ScopedTryLock stl (chainLock);
        if (stl.isLocked() && synthLayer != nullptr)
        {
            if (synthBuf.getNumSamples() < total)
                synthBuf.setSize (2, total, false, false, true);
            juce::AudioBuffer<float> sb (synthBuf.getArrayOfWritePointers(), 2, total);
            sb.clear();
            synthLayer->processBlock (sb, midiEcho);
            const float lvl = rawVal (ID::synthLevel);
            buffer.addFrom (0, 0, sb, 0, 0, total, lvl);
            buffer.addFrom (1, 0, sb, 1, 0, total, lvl);
        }
    }

    // FX chain
    if (rawVal (ID::chorusOn) > 0.5f)
    {
        chorus.setParams (rawVal (ID::chorusRate), rawVal (ID::chorusDepth), rawVal (ID::chorusMix));
        chorus.process (left, right, total);
    }
    if (rawVal (ID::delayOn) > 0.5f)
    {
        float time = rawVal (ID::delayTime);
        if (rawVal (ID::delaySync) > 0.5f && hostBpm > 1.0)
        {
            static const double beats[] = { 4.0, 2.0, 1.0, 1.5, 0.5, 0.75, 1.0/3.0, 0.25 };
            time = (float) (beats[(int) rawVal (ID::delayDiv)] * 60.0 / hostBpm);
        }
        delay.setParams (time, rawVal (ID::delayFB), rawVal (ID::delayMix));
        delay.process (left, right, total);
    }
    if (rawVal (ID::tremOn) > 0.5f)
    {
        tremolo.setParams (rawVal (ID::tremRate), rawVal (ID::tremDepth));
        tremolo.process (left, right, total);
    }

    // external VST3 inserts (end of chain, before master volume).
    // Try-lock: if the UI is editing the chain this block, skip inserts
    // rather than block the audio thread.
    if (buffer.getNumChannels() >= 2)
    {
        const juce::ScopedTryLock stl (chainLock);
        if (stl.isLocked() && ! insertChain.isEmpty())
        {
            juce::MidiBuffer noMidi;
            for (auto* p : insertChain)
                p->processBlock (buffer, noMidi);
        }
    }

    // master volume
    const float gain = juce::Decibels::decibelsToGain (rawVal (ID::masterVol));
    buffer.applyGain (gain);
    if (buffer.getNumChannels() == 1)
        juce::FloatVectorOperations::multiply (right, gain, total);

    // scope feed (mono sum of the actual output, post everything)
    if (buffer.getNumChannels() > 1)
    {
        if (scratch.getNumSamples() < total) scratch.setSize (1, total, false, false, true);
        float* mono = scratch.getWritePointer (0);
        for (int i = 0; i < total; ++i)
            mono[i] = 0.5f * (left[i] + right[i]);
        scopeFifo.push (mono, total);
    }
    else
        scopeFifo.push (left, total);

    activeVoices.store (engine.activeVoiceCount());
}

void EightyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("midiMap"), nullptr);
    state.appendChild (midiLearn.toValueTree(), nullptr);
    state.removeChild (state.getChildWithName ("insertChain"), nullptr);
    state.appendChild (chainToValueTree(), nullptr);
    state.removeChild (state.getChildWithName ("synthLayer"), nullptr);
    {
        juce::ValueTree s ("synthLayer");
        const juce::ScopedLock sl (chainLock);
        if (synthLayer != nullptr)
            s.appendChild (pluginInstanceToTree (*synthLayer), nullptr);
        state.appendChild (s, nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void EightyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        midiLearn.fromValueTree (state.getChildWithName ("midiMap"));
        state.removeChild (state.getChildWithName ("midiMap"), nullptr);

        auto chain = state.getChildWithName ("insertChain");
        state.removeChild (chain, nullptr);
        auto synth = state.getChildWithName ("synthLayer");
        state.removeChild (synth, nullptr);
        apvts.replaceState (state);

        // plugin instantiation must happen on the message thread
        juce::MessageManager::callAsync (
            [safeThis = juce::WeakReference<EightyProcessor> (this), chain, synth]
            {
                if (safeThis != nullptr)
                    safeThis->restoreChainFromValueTree (chain, synth);
            });
    }
}

juce::AudioProcessorEditor* EightyProcessor::createEditor()
{
    return new EightyEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EightyProcessor();
}
