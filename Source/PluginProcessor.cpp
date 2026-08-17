#include "PluginProcessor.h"
#include "PluginEditor.h"

EightyProcessor::EightyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", eighty::createParameterLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            raw[rp->paramID] = apvts.getRawParameterValue (rp->paramID);

    // per-key engine map default: CS-80 below middle C, JP-8 above
    fillKeyZones ([] (int note) { return (uint8_t) (note < 60 ? 0 : 1); });

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
    // The oversampler's decimation filters are linear phase, so they cost a
    // real group delay the host should compensate for (~27 samples at 8x).
    int latency = engine.latencySamples();
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

// A track is one string: steps separated by ';', chord notes by ',', each
// note as "pitch:velocity" (0-127). An empty step is a rest. A step that
// spans more than one step is prefixed "<hold>@" - absent means 1, so
// patterns saved before hold existed still load.
static juce::String seqTrackToString (const eighty::SynthEngine::SeqTrack& track)
{
    juce::String s;
    for (int i = 0; i < eighty::SynthEngine::SeqTrack::kSteps; ++i)
    {
        if (i > 0) s << ";";
        const auto& st = track.steps[(size_t) i];
        if (st.count > 0 && st.hold > 1) s << (int) st.hold << "@";
        for (uint8_t n = 0; n < st.count; ++n)
        {
            if (n > 0) s << ",";
            s << (int) st.note[n] << ":" << (int) st.vel[n];
        }
    }
    return s;
}

static void seqTrackFromString (const juce::String& s, eighty::SynthEngine::SeqTrack& track)
{
    using Step = eighty::SynthEngine::SeqStep;
    auto steps = juce::StringArray::fromTokens (s, ";", "");
    for (int i = 0; i < eighty::SynthEngine::SeqTrack::kSteps; ++i)
    {
        Step step;
        if (i < steps.size() && steps[i].isNotEmpty())
        {
            auto body = steps[i];
            int hold = 1;
            if (body.contains ("@"))
            {
                hold = body.upToFirstOccurrenceOf ("@", false, false).getIntValue();
                body = body.fromFirstOccurrenceOf ("@", false, false);
            }
            for (auto tok : juce::StringArray::fromTokens (body, ",", ""))
            {
                const int note = tok.upToFirstOccurrenceOf (":", false, false).getIntValue();
                const int vel  = tok.fromFirstOccurrenceOf (":", false, false).getIntValue();
                step.add (note, (float) juce::jlimit (1, 127, vel) / 127.f);
            }
            step.hold = (uint8_t) juce::jlimit (1, Step::kMaxHold, hold);
        }
        track.steps[(size_t) i].clear();
        track.steps[(size_t) i] = step;
    }
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
    {
        static const int factors[] = { 1, 2, 4, 8 };
        engine.setOversample (factors[juce::jlimit (0, 3, (int) rawVal (ID::oversample))]);
    }
    engine.prepare (sampleRate, juce::jmax (samplesPerBlock, 32));
    recorder.prepare (sampleRate);   // stops a take in progress: the rate changed
    chorus.prepare (sampleRate, samplesPerBlock);
    delay.prepare (sampleRate);
    tremolo.prepare (sampleRate);
    limiter.prepare (sampleRate);
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

    vp.sub[modelCS80].level   = rawVal (ID::csSubLevel);
    vp.sub[modelCS80].octaves = (int) rawVal (ID::csSubOct) + 1;   // chip 0/1 -> 1/2 octaves
    vp.sub[modelCS80].wave    = (int) rawVal (ID::csSubWave);
    vp.sub[modelJP8].level    = rawVal (ID::jpSubLevel);
    vp.sub[modelJP8].octaves  = (int) rawVal (ID::jpSubOct) + 1;
    vp.sub[modelJP8].wave     = (int) rawVal (ID::jpSubWave);

    vp.sineLevel    = rawVal (ID::csSineLevel);

    vp.hpfCutoff    = rawVal (ID::hpfCutoff);
    vp.lpfCutoff    = rawVal (ID::lpfCutoff);
    vp.resonance    = rawVal (ID::resonance);
    vp.hpfRes       = rawVal (ID::hpfRes);
    vp.filterEnvAmt = rawVal (ID::filterEnvAmt);
    vp.keyTrack     = rawVal (ID::keyTrack);
    vp.filterDrive  = rawVal (ID::filterDrive);

    vp.ch2.cutoffOct = rawVal (ID::csCh2Cut);
    vp.ch2.res       = rawVal (ID::csCh2Res);
    vp.ch2.envAmt    = rawVal (ID::csCh2Env);
    vp.ch2.timeScale = rawVal (ID::csCh2Time);

    vp.ring.on     = rawVal (ID::ringOn) > 0.5f;
    vp.ring.depth  = rawVal (ID::ringDepth);
    vp.ring.rateHz = rawVal (ID::ringRate);
    vp.ring.envAmt = rawVal (ID::ringEnvAmt);
    vp.ring.attack = rawVal (ID::ringAtk);
    vp.ring.decay  = rawVal (ID::ringDec);

    vp.brilliance   = juce::jlimit (-1.f, 1.f, rawVal (ID::brilliance) + gestureBright);
    vp.resOffset    = rawVal (ID::resOffset);
    vp.velBendSemis = rawVal (ID::velBend);

    vp.fA = rawVal (ID::fEnvA); vp.fD = rawVal (ID::fEnvD);
    vp.fS = rawVal (ID::fEnvS); vp.fR = rawVal (ID::fEnvR);
    vp.fIL = rawVal (ID::fEnvIL); vp.fAL = rawVal (ID::fEnvAL);
    vp.aA = rawVal (ID::aEnvA); vp.aD = rawVal (ID::aEnvD);
    vp.aS = rawVal (ID::aEnvS); vp.aR = rawVal (ID::aEnvR);
    vp.velToAmp    = rawVal (ID::velToAmp);
    vp.velToFilter = rawVal (ID::velToFilter);

    // Render quality. Changing the factor only reassigns rates, so it is
    // safe here on the audio thread; the latency it costs is republished
    // to the host when it changes.
    {
        static const int factors[] = { 1, 2, 4, 8 };
        const int want = factors[juce::jlimit (0, 3, (int) rawVal (ID::oversample))];
        if (want != engine.oversampleFactor())
        {
            engine.setOversample (want);
            updateChainLatency();
        }
    }

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
    jp.vco2Low       = rawVal (ID::jpVco2Low) > 0.5f;
    jp.sync          = rawVal (ID::jpSync) > 0.5f;
    jp.xmod          = rawVal (ID::jpXmod);
    jp.hpf           = rawVal (ID::jpHpf);
    jp.lpf           = rawVal (ID::jpLpf);
    jp.res           = rawVal (ID::jpRes);
    jp.drive         = rawVal (ID::jpDrive);
    jp.slope24       = rawVal (ID::jpSlope24) > 0.5f;
    jp.envAmt        = rawVal (ID::jpEnvAmt);
    jp.envInv        = rawVal (ID::jpEnvInv) > 0.5f;
    jp.keyTrack      = rawVal (ID::jpKeyTrk);
    jp.fA = rawVal (ID::jpFEnvA); jp.fD = rawVal (ID::jpFEnvD);
    jp.fS = rawVal (ID::jpFEnvS); jp.fR = rawVal (ID::jpFEnvR);
    jp.aA = rawVal (ID::jpAEnvA); jp.aD = rawVal (ID::jpAEnvD);
    jp.aS = rawVal (ID::jpAEnvS); jp.aR = rawVal (ID::jpAEnvR);

    es.arpMult    = (int) rawVal (ID::arpMult);
    es.strumPluck = rawVal (ID::tpArtic) > 0.5f;

    es.engineMode = (int) rawVal (ID::engineMode);
    es.splitPoint = (int) rawVal (ID::splitPoint);
    es.csLow      = rawVal (ID::splitCsLow) > 0.5f;
    for (int i = 0; i < 128; ++i)
        es.keyMap[(size_t) i] = keyZones[(size_t) i].load (std::memory_order_relaxed);

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

    // Output mixer, on top of the balance: a level per sound source with
    // mute and solo. Solo anywhere silences every strip that is not soloed.
    // The engine gains are smoothed per voice at control rate, so a mute is
    // a fast fade rather than a click.
    {
        const bool anySolo = rawVal (ID::csSolo)    > 0.5f
                          || rawVal (ID::jpSolo)    > 0.5f
                          || rawVal (ID::synthSolo) > 0.5f;
        auto strip = [anySolo] (float level, float mute, float solo)
        { return (mute > 0.5f || (anySolo && solo < 0.5f)) ? 0.f : level; };

        vp.csGain *= strip (rawVal (ID::csLevel), rawVal (ID::csMute), rawVal (ID::csSolo));
        vp.jpGain *= strip (rawVal (ID::jpLevel), rawVal (ID::jpMute), rawVal (ID::jpSolo));
        synthGain  = strip (rawVal (ID::synthLevel), rawVal (ID::synthMute),
                            rawVal (ID::synthSolo));
    }

    vp.drift        = rawVal (ID::drift);
    vp.stereoSpread = rawVal (ID::stereoSpread);
    vp.glide[modelCS80].time = rawVal (ID::csGlideTime);
    vp.glide[modelCS80].mode = (int) rawVal (ID::csGlideMode);
    vp.glide[modelJP8].time  = rawVal (ID::jpGlideTime);
    vp.glide[modelJP8].mode  = (int) rawVal (ID::jpGlideMode);
    vp.masterTuneCents = rawVal (ID::masterTune);

    es.csVoice.mode         = (SynthEngine::Mode) (int) rawVal (ID::csVoiceMode);
    es.csVoice.polyVoices   = (int) rawVal (ID::csPolyVoices);
    es.csVoice.unisonCount  = (int) rawVal (ID::csUnisonCount);
    es.csVoice.unisonDetune = rawVal (ID::csUnisonDetune);
    es.jpVoice.mode         = (SynthEngine::Mode) (int) rawVal (ID::jpVoiceMode);
    es.jpVoice.polyVoices   = (int) rawVal (ID::jpPolyVoices);
    es.jpVoice.unisonCount  = (int) rawVal (ID::jpUnisonCount);
    es.jpVoice.unisonDetune = rawVal (ID::jpUnisonDetune);
    es.bendRange    = (int) rawVal (ID::bendRange);

    es.arpMode    = (int) rawVal (ID::arpMode);
    es.arpDiv     = (int) rawVal (ID::arpDiv);
    es.arpOctaves = (int) rawVal (ID::arpOctaves);
    es.arpGate    = rawVal (ID::arpGate);

    // sequencer track settings (the pattern itself lives in the engine and
    // is edited directly by the grid)
    {
        auto& seq = engine.seq;
        seq.recTrack = (int) rawVal (ID::seqTrack);
        seq.tracks[0].mute   = rawVal (ID::seqAMute) > 0.5f;
        seq.tracks[1].mute   = rawVal (ID::seqBMute) > 0.5f;
        seq.tracks[0].engine = (int) rawVal (ID::seqAEng);
        seq.tracks[1].engine = (int) rawVal (ID::seqBEng);
        seq.tracks[0].length = (int) rawVal (ID::seqALen);
        seq.tracks[1].length = (int) rawVal (ID::seqBLen);
    }

    es.lfoRate  = rawVal (ID::lfoRate);
    es.lfoWave  = (int) rawVal (ID::lfoWave);
    es.lfoDelay = rawVal (ID::lfoDelay);
    es.pwmRate  = rawVal (ID::pwmRate);

    // Tempo: the panel TEMPO drives the arp, the sequencer and the synced
    // delay. SYNC hands that over to the host, but only when the host really
    // reports a tempo - in a standalone build there is none, and the knob
    // silently doing nothing would be the wrong answer.
    {
        const bool followHost = rawVal (ID::arpSync) > 0.5f && hostBpmValid;
        curBpm  = followHost ? hostBpm : (double) rawVal (ID::tempo);
        es.bpm  = curBpm;
        uiBpm.store ((float) curBpm);
        uiBpmFromHost.store (followHost);
    }

    // edge-detected toggles
    // Arp and the step sequencer both want the step clock, so whichever
    // just turned on forces the other off, params included (so the UI LEDs
    // stay honest). REC and PLAY *do* coexist: recording writes at the
    // cursor while playback runs, which is how you overdub the second
    // track against the first.
    const bool arpRaw[2] = { rawVal (ID::csArpOn) > 0.5f, rawVal (ID::jpArpOn) > 0.5f };
    bool seqRecRaw  = rawVal (ID::seqRec)  > 0.5f;
    bool seqPlayRaw = rawVal (ID::seqPlay) > 0.5f;


    const bool seqRecRising  = seqRecRaw  && ! lastSeqRec;
    const bool seqPlayRising = seqPlayRaw && ! lastSeqPlay;

    auto forceOff = [this] (const char* id) { apvts.getParameter (id)->setValueNotifyingHost (0.f); };

    // The arpeggiator and the sequencer used to switch each other off. They
    // no longer do: with both running, each track's chord becomes the figure
    // its own arpeggiator walks, so a sequence of chords can be turned into a
    // sequence of arpeggios with one switch - and live playing still
    // arpeggiates over the top of it.
    juce::ignoreUnused (seqRecRising, seqPlayRising);

    // Each engine's arp switch is handed over on its own edge, so flipping
    // one never disturbs what the other is doing.
    for (int m = 0; m < 2; ++m)
    {
        if (arpRaw[m] == lastArpOn[m]) continue;
        es.arpOn[m] = arpRaw[m];
        engine.arpModeChanged (m, arpRaw[m]);
        lastArpOn[m] = arpRaw[m];
    }
    es.arpOn[0] = arpRaw[0];
    es.arpOn[1] = arpRaw[1];

    if (seqRecRaw != lastSeqRec)
    {
        if (seqRecRaw) engine.seqRecStart();
        else           engine.seqRec = false;
        lastSeqRec = seqRecRaw;
    }
    if (seqPlayRaw != lastSeqPlay)
    {
        if (seqPlayRaw) engine.seqPlayStart();
        else            engine.seqPlayStop();
        lastSeqPlay = seqPlayRaw;
    }

    // The engine auto-stops recording once the 16th step is committed;
    // resync the REC param/LED when that happens outside the UI.
    if (! engine.seqRec && lastSeqRec)
    {
        lastSeqRec = false;
        forceOff (ID::seqRec);
    }

    const bool holdOn = rawVal (ID::hold) > 0.5f;
    if (holdOn != lastHold)
    {
        engine.setHold (holdOn);
        lastHold = holdOn;
    }
}

// One trackpad gesture, now on the audio thread. Everything here goes
// through the engine's ordinary note path - the strum is not a special case
// in the voice allocator, only a different set of fingers driving it.
void EightyProcessor::applyGesture (const GestureMsg& g)
{
    switch (g.type)
    {
        case GestureMsg::kArm:
            engine.strumArm (g.flag, g.dir != 0);
            if (! g.flag)
            {
                gestureBright = 0.f;
                engine.setChannelPressure (0.f);
            }
            break;

        case GestureMsg::kNote:
            engine.strumNote (g.lane, g.note, g.value, g.model);
            break;

        case GestureMsg::kLaneOff:
            engine.strumLaneOff (g.lane, g.flag);
            break;

        case GestureMsg::kPatternStep:
            engine.strumPatternStep (g.lane, g.dir);
            break;

        case GestureMsg::kRibbon:
            engine.setRibbon (g.model, g.value);
            break;

        case GestureMsg::kPressure:
            engine.setChannelPressure (juce::jlimit (0.f, 1.f, g.value));
            break;

        // The cross axis riding BRILLIANCE is an offset over the panel knob,
        // not a write to it: the finger is a performance move, and it should
        // no more end up in the patch than the pitch wheel does.
        case GestureMsg::kBright:
            gestureBright = juce::jlimit (-1.f, 1.f, g.value * 2.f - 1.f);
            break;
    }
}

void EightyProcessor::handleMidiEvent (const juce::MidiMessage& m)
{
    auto markHeld = [this] (int note, int vel)
    {
        heldNoteVel[(size_t) juce::jlimit (0, 127, note)]
            .store ((uint8_t) vel, std::memory_order_relaxed);
    };

    if (m.isNoteOn())
    {
        markHeld (m.getNoteNumber(), juce::jlimit (1, 127, (int) m.getVelocity()));
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    }
    else if (m.isNoteOff())
    {
        markHeld (m.getNoteNumber(), 0);
        engine.noteOff (m.getNoteNumber());
    }
    else if (m.isPitchWheel())
        engine.setPitchBend (m.getPitchWheelValue());
    else if (m.isChannelPressure())
        engine.setChannelPressure ((float) m.getChannelPressureValue() / 127.f);
    else if (m.isAftertouch())
        engine.setPolyPressure (m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.f);
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        for (int i = 0; i < 128; ++i) markHeld (i, 0);
        engine.allNotesOff();
    }
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

    hostBpmValid = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 1.0) { hostBpm = *bpm; hostBpmValid = true; }

    // Trackpad gestures first, so a strum struck between blocks is already
    // in the voice pool by the time this block's parameters are applied to
    // it. Timestamps land at the top of the block, as the UI keyboard's do.
    engine.echoBase = 0;
    gestures.drain ([this] (const GestureMsg& g) { applyGesture (g); });

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
            buffer.addFromWithRamp (0, 0, sb.getReadPointer (0), total, lastSynthGain, synthGain);
            buffer.addFromWithRamp (1, 0, sb.getReadPointer (1), total, lastSynthGain, synthGain);
            lastSynthGain = synthGain;
        }
    }

    // FX chain
    if (rawVal (ID::chorusOn) > 0.5f)
    {
        chorus.setParams (rawVal (ID::chorusRate), rawVal (ID::chorusDepth),
                          rawVal (ID::chorusMix), (int) rawVal (ID::chorusMode));
        chorus.process (left, right, total);
    }
    if (rawVal (ID::delayOn) > 0.5f)
    {
        float time = rawVal (ID::delayTime);
        if (rawVal (ID::delaySync) > 0.5f && curBpm > 1.0)
        {
            static const double beats[] = { 4.0, 2.0, 1.0, 1.5, 0.5, 0.75, 1.0/3.0, 0.25 };
            time = (float) (beats[(int) rawVal (ID::delayDiv)] * 60.0 / curBpm);
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

    // stereo width (M/S): 0 = mono, 1 = neutral, 2 = double-wide. Widening
    // the detuned voice spread is most of what makes a big patch feel big,
    // so this sits right before the limiter and stays honest about level.
    if (buffer.getNumChannels() > 1)
    {
        const float width = rawVal (ID::stereoWidth);
        if (std::abs (width - 1.f) > 0.001f)
            for (int i = 0; i < total; ++i)
            {
                const float m = 0.5f * (left[i] + right[i]);
                const float s = 0.5f * (left[i] - right[i]) * width;
                left[i]  = m + s;
                right[i] = m - s;
            }
    }

    // output limiter (always in circuit as a safety; the knob adds drive)
    limiter.setDrive (rawVal (ID::limitDrive));
    limiter.process (left, right, total);

    // scope feed (stereo, post everything: waveform + lissajous)
    scopeFifo.push (left, right, total);

    // The recorder taps exactly here, so what lands in the file is what the
    // scopes draw and what you hear - not an earlier, tidier version of it.
    recorder.write (left, right, total);

    activeVoices.store (engine.activeVoiceCount());
}

// ------------------------------------------------------------- presets
// The "sound": every parameter plus the two things the engine owns outside
// the APVTS - the per-key engine map and the sequencer pattern.
juce::ValueTree EightyProcessor::soundToValueTree()
{
    auto state = apvts.copyState();
    for (auto* name : { "midiMap", "keyZones", "seqPattern", "insertChain", "synthLayer" })
        state.removeChild (state.getChildWithName (name), nullptr);

    {
        std::string zones (128, '0');
        for (int i = 0; i < 128; ++i)
            zones[(size_t) i] = (char) ('0' + getKeyZone (i));
        juce::ValueTree kz ("keyZones");
        kz.setProperty ("map", juce::String (zones), nullptr);
        state.appendChild (kz, nullptr);
    }
    {
        juce::ValueTree sp ("seqPattern");
        for (int t = 0; t < eighty::SynthEngine::StepSeq::kTracks; ++t)
        {
            juce::ValueTree tr ("track");
            tr.setProperty ("steps", seqTrackToString (engine.seq.tracks[(size_t) t]), nullptr);
            sp.appendChild (tr, nullptr);
        }
        state.appendChild (sp, nullptr);
    }
    return state;
}

void EightyProcessor::soundFromValueTree (const juce::ValueTree& tree)
{
    auto state = tree.createCopy();

    auto kz = state.getChildWithName ("keyZones");
    if (kz.isValid())
    {
        const auto zones = kz.getProperty ("map").toString();
        for (int i = 0; i < 128 && i < zones.length(); ++i)
            setKeyZone (i, (uint8_t) juce::jlimit (0, 2, (int) (zones[i] - '0')));
    }
    state.removeChild (kz, nullptr);

    auto sp = state.getChildWithName ("seqPattern");
    if (sp.isValid())
    {
        int i = 0;
        for (auto tr : sp)
        {
            if (i >= eighty::SynthEngine::StepSeq::kTracks) break;
            if (tr.hasType ("track"))
                seqTrackFromString (tr.getProperty ("steps").toString(),
                                    engine.seq.tracks[(size_t) i]);
            ++i;
        }
    }
    state.removeChild (sp, nullptr);

    apvts.replaceState (state);
}

// ------------------------------------------------------ factory presets
int EightyProcessor::getNumFactoryPresets()
{
    return (int) eighty::factoryPresets().size();
}

juce::String EightyProcessor::getFactoryPresetName (int index)
{
    const auto& list = eighty::factoryPresets();
    return index >= 0 && index < (int) list.size() ? juce::String (list[(size_t) index].name)
                                                   : juce::String();
}

void EightyProcessor::loadFactoryPreset (int index)
{
    const auto& list = eighty::factoryPresets();
    if (index < 0 || index >= (int) list.size()) return;
    const auto& preset = list[(size_t) index];

    engine.allNotesOff();

    // Defaults first, then the preset's overrides: that is what makes these
    // clean starting points instead of a diff against whatever was loaded
    // before. Every parameter is touched, so nothing can survive the switch.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : preset.params)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));

    // The pattern and the key map live outside the APVTS, so they need
    // clearing by hand or the last sound's sequence plays under the new one.
    for (int t = 0; t < eighty::SynthEngine::StepSeq::kTracks; ++t)
        engine.seqClearTrack (t);
    fillKeyZones ([] (int note) { return (uint8_t) (note < 60 ? 0 : 1); });

    presetName = preset.name;
    presetDirty = false;
}

juce::File EightyProcessor::presetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Eighty").getChildFile ("Presets");
}

// ------------------------------------------------------- audio recording
juce::File EightyProcessor::recordingsFolder()
{
    return presetFolder().getSiblingFile ("Recordings");
}

bool EightyProcessor::startRecording (juce::String& error)
{
    // Named for the patch and the moment, so a folder full of takes is still
    // navigable a week later and nothing ever overwrites anything.
    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M-%S");
    const auto name = juce::File::createLegalFileName (
        "Eighty " + presetName + " " + stamp + ".wav");
    return recorder.start (recordingsFolder().getChildFile (name), error);
}

juce::Array<juce::File> EightyProcessor::presetFiles() const
{
    juce::Array<juce::File> files;
    presetFolder().findChildFiles (files, juce::File::findFiles, false, "*.eighty");
    files.sort();
    return files;
}

bool EightyProcessor::savePreset (const juce::String& name, juce::String& error)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) { error = "Please give the preset a name"; return false; }

    auto folder = presetFolder();
    const auto result = folder.createDirectory();
    if (result.failed()) { error = result.getErrorMessage(); return false; }

    juce::ValueTree root ("EightyPreset");
    root.setProperty ("name", clean, nullptr);
    root.setProperty ("version", 1, nullptr);
    root.appendChild (soundToValueTree(), nullptr);

    auto file = folder.getChildFile (clean + ".eighty");
    auto xml = root.createXml();
    if (xml == nullptr || ! xml->writeTo (file))
    {
        error = "Couldn't write " + file.getFullPathName();
        return false;
    }
    presetName = clean;
    presetDirty = false;
    return true;
}

bool EightyProcessor::loadPreset (const juce::File& file, juce::String& error)
{
    auto xml = juce::parseXML (file);
    if (xml == nullptr) { error = "Couldn't read " + file.getFileName(); return false; }

    auto root = juce::ValueTree::fromXml (*xml);
    auto sound = root.hasType ("EightyPreset") ? root.getChild (0) : root;
    if (! sound.isValid()) { error = file.getFileName() + " is not an Eighty preset"; return false; }

    engine.allNotesOff();
    soundFromValueTree (sound);
    presetName = file.getFileNameWithoutExtension();
    presetDirty = false;
    return true;
}

void EightyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = soundToValueTree();
    state.setProperty ("presetName", presetName, nullptr);
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

        if (state.hasProperty ("presetName"))
            presetName = state.getProperty ("presetName").toString();
        presetDirty = false;

        auto chain = state.getChildWithName ("insertChain");
        state.removeChild (chain, nullptr);
        auto synth = state.getChildWithName ("synthLayer");
        state.removeChild (synth, nullptr);
        soundFromValueTree (state);

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
