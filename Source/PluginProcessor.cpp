#include "PluginProcessor.h"
#include "PluginEditor.h"

EightyProcessor::EightyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", eighty::createParameterLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            raw[rp->paramID] = apvts.getRawParameterValue (rp->paramID);

    formatManager.addDefaultFormats();   // VST3 hosting (JUCE_PLUGINHOST_VST3)
    if (auto xml = juce::parseXML (pluginCacheFile()))
        knownPlugins.recreateFromXml (*xml);
}

// ------------------------------------------------ external insert chain
juce::File EightyProcessor::pluginCacheFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Eighty").getChildFile ("pluginCache.xml");
}

void EightyProcessor::rescanPlugins()
{
    knownPlugins.clear();
    juce::VST3PluginFormat format;
    juce::PluginDirectoryScanner scanner (knownPlugins, format,
                                          format.getDefaultLocationsToSearch(),
                                          true, juce::File());
    juce::String progressName;
    while (scanner.scanNextFile (true, progressName)) {}

    if (auto xml = knownPlugins.createXml())
    {
        pluginCacheFile().getParentDirectory().createDirectory();
        xml->writeTo (pluginCacheFile());
    }
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

juce::ValueTree EightyProcessor::chainToValueTree() const
{
    juce::ValueTree t ("insertChain");
    const juce::ScopedLock sl (chainLock);
    for (auto* p : insertChain)
    {
        juce::ValueTree e ("plugin");
        if (auto descXml = p->getPluginDescription().createXml())
            e.setProperty ("desc", descXml->toString(), nullptr);
        juce::MemoryBlock mb;
        p->getStateInformation (mb);
        e.setProperty ("state", mb.toBase64Encoding(), nullptr);
        t.appendChild (e, nullptr);
    }
    return t;
}

void EightyProcessor::restoreChainFromValueTree (juce::ValueTree state)
{
    // clear existing
    while (getNumInserts() > 0)
        removeInsert (0);

    for (auto e : state)
    {
        if (! e.hasType ("plugin")) continue;
        juce::PluginDescription desc;
        if (auto descXml = juce::parseXML (e.getProperty ("desc").toString()))
            if (! desc.loadFromXml (*descXml))
                continue;
        juce::String error;
        if (addInsert (desc, error))
        {
            juce::MemoryBlock mb;
            if (mb.fromBase64Encoding (e.getProperty ("state").toString()))
                if (auto* inst = getInsert (getNumInserts() - 1))
                    inst->setStateInformation (mb.getData(), (int) mb.getSize());
        }
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

    {
        const juce::ScopedLock sl (chainLock);
        for (auto* p : insertChain)
        {
            p->releaseResources();
            p->setPlayConfigDetails (2, 2, sampleRate, samplesPerBlock);
            p->prepareToPlay (sampleRate, samplesPerBlock);
        }
    }
    updateChainLatency();
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
    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, total, (int) meta.samplePosition);
        if (t > pos)
        {
            engine.render (left + pos, right + pos, t - pos);
            pos = t;
        }
        handleMidiEvent (meta.getMessage());
    }
    if (pos < total)
        engine.render (left + pos, right + pos, total - pos);
    midi.clear();

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
        apvts.replaceState (state);

        // plugin instantiation must happen on the message thread
        juce::MessageManager::callAsync (
            [safeThis = juce::WeakReference<EightyProcessor> (this), chain]
            {
                if (safeThis != nullptr)
                    safeThis->restoreChainFromValueTree (chain);
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
