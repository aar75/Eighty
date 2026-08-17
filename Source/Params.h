#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// All parameter IDs. Grouped roughly as the UI sections.
namespace ID
{
    // Oscillator channels I & II (CS-80 style: one VCO core per channel,
    // saw + PWM pulse derived from the same core, mixed per-channel)
    inline constexpr const char* osc1On        = "osc1On";
    inline constexpr const char* osc1Foot      = "osc1Foot";
    inline constexpr const char* osc1Fine      = "osc1Fine";
    inline constexpr const char* osc1Saw       = "osc1Saw";
    inline constexpr const char* osc1Pulse     = "osc1Pulse";
    inline constexpr const char* osc1PW        = "osc1PW";
    inline constexpr const char* osc1PWM       = "osc1PWM";
    inline constexpr const char* osc1Level     = "osc1Level";

    inline constexpr const char* osc2On        = "osc2On";
    inline constexpr const char* osc2Foot      = "osc2Foot";
    inline constexpr const char* osc2Semi      = "osc2Semi";
    inline constexpr const char* osc2Fine      = "osc2Fine";
    inline constexpr const char* osc2Saw       = "osc2Saw";
    inline constexpr const char* osc2Pulse     = "osc2Pulse";
    inline constexpr const char* osc2PW        = "osc2PW";
    inline constexpr const char* osc2PWM       = "osc2PWM";
    inline constexpr const char* osc2Level     = "osc2Level";

    inline constexpr const char* noiseLevel    = "noiseLevel";
    inline constexpr const char* pwmRate       = "pwmRate";

    // Sub-oscillator, one per engine: a square or triangle an octave or two
    // below the engine's first oscillator, straight into the filter.
    inline constexpr const char* csSubLevel = "csSubLevel";
    inline constexpr const char* csSubOct   = "csSubOct";    // -1 / -2 octaves
    inline constexpr const char* csSubWave  = "csSubWave";   // Square / Triangle
    inline constexpr const char* jpSubLevel = "jpSubLevel";
    inline constexpr const char* jpSubOct   = "jpSubOct";
    inline constexpr const char* jpSubWave  = "jpSubWave";

    // CS-80 sine: routed around the filters into the VCA mixer, as on the
    // original, so a patch keeps its fundamental under a closed filter.
    inline constexpr const char* csSineLevel = "csSineLevel";

    // Filter (CS-80 topology: 12 dB/oct HPF into 12 dB/oct LPF, both resonant)
    inline constexpr const char* hpfCutoff     = "hpfCutoff";
    inline constexpr const char* lpfCutoff     = "lpfCutoff";
    inline constexpr const char* resonance     = "resonance";
    inline constexpr const char* hpfRes        = "hpfRes";
    inline constexpr const char* filterEnvAmt  = "filterEnvAmt";
    inline constexpr const char* keyTrack      = "keyTrack";
    inline constexpr const char* filterDrive   = "filterDrive";

    // CS-80 channel II offsets. The real machine is two complete synths;
    // channel II here runs the same panel with its own filter, its own
    // envelope generators and these four offsets against channel I.
    inline constexpr const char* csCh2Cut  = "csCh2Cut";    // octaves
    inline constexpr const char* csCh2Res  = "csCh2Res";
    inline constexpr const char* csCh2Env  = "csCh2Env";
    inline constexpr const char* csCh2Time = "csCh2Time";   // envelope speed ratio

    // Ring modulator (CS-80): sine carrier, own AD envelope, envelope also
    // drives the carrier speed.
    inline constexpr const char* ringOn     = "ringOn";
    inline constexpr const char* ringDepth  = "ringDepth";
    inline constexpr const char* ringRate   = "ringRate";
    inline constexpr const char* ringEnvAmt = "ringEnvAmt";
    inline constexpr const char* ringAtk    = "ringAtk";
    inline constexpr const char* ringDec    = "ringDec";

    // Global performance macros + render quality
    inline constexpr const char* brilliance = "brilliance";
    inline constexpr const char* resOffset  = "resOffset";
    inline constexpr const char* velBend    = "velBend";     // Touch Response initial bend
    inline constexpr const char* oversample = "oversample";  // Off / 2x / 4x / 8x

    // Envelopes
    inline constexpr const char* fEnvA = "fEnvA";
    inline constexpr const char* fEnvD = "fEnvD";
    inline constexpr const char* fEnvS = "fEnvS";
    inline constexpr const char* fEnvR = "fEnvR";
    inline constexpr const char* fEnvIL = "fEnvIL";   // CS-80 initial level
    inline constexpr const char* fEnvAL = "fEnvAL";   // CS-80 attack level
    inline constexpr const char* aEnvA = "aEnvA";
    inline constexpr const char* aEnvD = "aEnvD";
    inline constexpr const char* aEnvS = "aEnvS";
    inline constexpr const char* aEnvR = "aEnvR";
    inline constexpr const char* velToAmp    = "velToAmp";
    inline constexpr const char* velToFilter = "velToFilter";

    // LFO (single global sub-oscillator, as on the CS-80)
    inline constexpr const char* lfoRate    = "lfoRate";
    inline constexpr const char* lfoWave    = "lfoWave";
    inline constexpr const char* lfoDelay   = "lfoDelay";
    inline constexpr const char* lfoToPitch = "lfoToPitch";
    inline constexpr const char* lfoToFilter= "lfoToFilter";
    inline constexpr const char* lfoToAmp   = "lfoToAmp";

    // Touch (aftertouch response + simulated pressure for computer keys)
    inline constexpr const char* touchRise     = "touchRise";
    inline constexpr const char* touchToVib    = "touchToVib";
    inline constexpr const char* touchToBright = "touchToBright";
    inline constexpr const char* touchToLevel  = "touchToLevel";

    // Voices (per-engine: CS-80 and JP-8 run independent mode/voice counts)
    inline constexpr const char* csVoiceMode    = "csVoiceMode";   // Poly / Mono / Legato / Unison
    inline constexpr const char* csPolyVoices   = "csPolyVoices";
    inline constexpr const char* csUnisonCount  = "csUnisonCount";
    inline constexpr const char* csUnisonDetune = "csUnisonDetune";
    inline constexpr const char* jpVoiceMode    = "jpVoiceMode";
    inline constexpr const char* jpPolyVoices   = "jpPolyVoices";
    inline constexpr const char* jpUnisonCount  = "jpUnisonCount";
    inline constexpr const char* jpUnisonDetune = "jpUnisonDetune";
    inline constexpr const char* stereoSpread = "stereoSpread";
    inline constexpr const char* drift        = "drift";       // analog inconsistency amount
    // Glide is per-engine: the CS-80 and JP-8 cards portamento independently
    inline constexpr const char* csGlideTime  = "csGlideTime";
    inline constexpr const char* csGlideMode  = "csGlideMode"; // Off / Legato / Always
    inline constexpr const char* jpGlideTime  = "jpGlideTime";
    inline constexpr const char* jpGlideMode  = "jpGlideMode";
    inline constexpr const char* bendRange    = "bendRange";
    inline constexpr const char* hold         = "hold";

    // Tempo. One BPM value drives the arpeggiator, the step sequencer and the
    // synced delay; arpSync makes it follow the host instead, when the host
    // actually reports a tempo (a standalone build never does).
    inline constexpr const char* tempo      = "tempo";

    // Arpeggiator
    // Per engine: the CS-80 can arpeggiate a chord while the JP-8 holds it,
    // or the other way round. Everything else about the arp is shared.
    inline constexpr const char* csArpOn    = "csArpOn";
    inline constexpr const char* jpArpOn    = "jpArpOn";
    inline constexpr const char* arpMode    = "arpMode";
    inline constexpr const char* arpSync    = "arpSync";
    inline constexpr const char* arpDiv     = "arpDiv";
    inline constexpr const char* arpOctaves = "arpOctaves";
    // Arp notes per DIV step. x1 is the step clock itself; above that the arp
    // subdivides it, which is how an arpeggio runs *inside* a sequenced chord.
    inline constexpr const char* arpMult    = "arpMult";
    inline constexpr const char* arpGate    = "arpGate";

    // Step sequencer: 2 tracks x 16 steps, shares the arp's rate/div/sync/
    // gate, mutually exclusive with the arpeggiator. Live playing runs on
    // top of a running sequence.
    inline constexpr const char* seqRec   = "seqRec";
    inline constexpr const char* seqPlay  = "seqPlay";
    inline constexpr const char* seqTrack = "seqTrack";   // armed / edited track
    inline constexpr const char* seqAMute = "seqAMute";
    inline constexpr const char* seqBMute = "seqBMute";
    inline constexpr const char* seqAEng  = "seqAEng";    // Auto / CS-80 / JP-8
    inline constexpr const char* seqBEng  = "seqBEng";
    inline constexpr const char* seqALen  = "seqALen";
    inline constexpr const char* seqBLen  = "seqBLen";

    // Trackpad performance. The pad is treated as a playing surface rather
    // than a pointer: a strum across the notes you are holding, and the
    // ribbon the CS-80 panel never got. All of it lives behind a held key,
    // so nothing here changes how the synth behaves until you ask for it.
    inline constexpr const char* tpOn        = "tpOn";
    inline constexpr const char* tpKey       = "tpKey";        // Q / Space / `
    inline constexpr const char* tpLatch     = "tpLatch";      // key toggles, not holds
    inline constexpr const char* tpTarget    = "tpTarget";     // Chord / Pattern
    inline constexpr const char* tpSpan      = "tpSpan";       // octaves across the pad
    inline constexpr const char* tpForce     = "tpForce";      // stroke speed -> velocity
    inline constexpr const char* tpAxis      = "tpAxis";       // strum along X / Y
    inline constexpr const char* tpCross     = "tpCross";      // cross-axis destination
    inline constexpr const char* tpArtic     = "tpArtic";      // Hold / Pluck
    inline constexpr const char* tpLift      = "tpLift";       // Ring / Damp / Gesture
    inline constexpr const char* tpFingers   = "tpFingers";    // Per Engine / Both
    inline constexpr const char* tpFreeze    = "tpFreeze";     // freeze the pointer
    inline constexpr const char* tpMonitor   = "tpMonitor";    // on-screen touch monitor
    inline constexpr const char* tpRibbon    = "tpRibbon";     // ribbon key enabled
    inline constexpr const char* tpRibbonRng = "tpRibbonRng";  // semitones at the edge
    inline constexpr const char* tpRibbonAbs = "tpRibbonAbs";  // absolute, not relative

    // FX
    inline constexpr const char* chorusOn    = "chorusOn";
    inline constexpr const char* chorusMode  = "chorusMode";   // I / II / Ensemble
    inline constexpr const char* chorusRate  = "chorusRate";
    inline constexpr const char* chorusDepth = "chorusDepth";
    inline constexpr const char* chorusMix   = "chorusMix";
    inline constexpr const char* delayOn     = "delayOn";
    inline constexpr const char* delayTime   = "delayTime";
    inline constexpr const char* delaySync   = "delaySync";
    inline constexpr const char* delayDiv    = "delayDiv";
    inline constexpr const char* delayFB     = "delayFB";
    inline constexpr const char* delayMix    = "delayMix";
    inline constexpr const char* tremOn      = "tremOn";
    inline constexpr const char* tremRate    = "tremRate";
    inline constexpr const char* tremDepth   = "tremDepth";

    // Master
    inline constexpr const char* masterVol  = "masterVol";
    inline constexpr const char* masterTune = "masterTune";
    inline constexpr const char* limitDrive = "limitDrive";   // one-knob output limiter
    inline constexpr const char* stereoWidth = "stereoWidth"; // M/S width, 1 = neutral

    // Engine selection / keyboard split
    inline constexpr const char* engineMode = "engineMode";   // CS-80 / JP-8 / Split / Layer / Keys
    inline constexpr const char* splitPoint = "splitPoint";
    inline constexpr const char* splitCsLow = "splitCsLow";   // CS on the low side?
    inline constexpr const char* engineBalance = "engineBalance"; // CS <-> JP mix

    // Output mixer: one strip per sound source (CS-80 card, JP-8 card and the
    // hosted VST3 synth layer). Solo anywhere mutes every strip that is not
    // soloed, as on a desk.
    inline constexpr const char* csLevel    = "csLevel";
    inline constexpr const char* csMute     = "csMute";
    inline constexpr const char* csSolo     = "csSolo";
    inline constexpr const char* jpLevel    = "jpLevel";
    inline constexpr const char* jpMute     = "jpMute";
    inline constexpr const char* jpSolo     = "jpSolo";
    inline constexpr const char* synthLevel = "synthLevel";   // hosted VST3 synth layer
    inline constexpr const char* synthMute  = "synthMute";
    inline constexpr const char* synthSolo  = "synthSolo";

    // Jupiter-8 voice card
    inline constexpr const char* jpVco1Wave  = "jpVco1Wave";
    inline constexpr const char* jpVco1Range = "jpVco1Range";
    inline constexpr const char* jpPW        = "jpPW";
    inline constexpr const char* jpPWM       = "jpPWM";
    inline constexpr const char* jpMix       = "jpMix";
    inline constexpr const char* jpVco2Wave  = "jpVco2Wave";
    inline constexpr const char* jpVco2Range = "jpVco2Range";
    inline constexpr const char* jpVco2Semi  = "jpVco2Semi";
    inline constexpr const char* jpVco2Fine  = "jpVco2Fine";
    inline constexpr const char* jpVco2Low   = "jpVco2Low";   // VCO-2 as an LF modulator
    inline constexpr const char* jpSync      = "jpSync";
    inline constexpr const char* jpXmod      = "jpXmod";
    inline constexpr const char* jpHpf       = "jpHpf";
    inline constexpr const char* jpLpf       = "jpLpf";
    inline constexpr const char* jpRes       = "jpRes";
    inline constexpr const char* jpDrive     = "jpDrive";
    inline constexpr const char* jpSlope24   = "jpSlope24";
    inline constexpr const char* jpEnvAmt    = "jpEnvAmt";
    inline constexpr const char* jpEnvInv    = "jpEnvInv";    // ENV-1 polarity
    inline constexpr const char* jpKeyTrk    = "jpKeyTrk";
    inline constexpr const char* jpFEnvA = "jpFEnvA";
    inline constexpr const char* jpFEnvD = "jpFEnvD";
    inline constexpr const char* jpFEnvS = "jpFEnvS";
    inline constexpr const char* jpFEnvR = "jpFEnvR";
    inline constexpr const char* jpAEnvA = "jpAEnvA";
    inline constexpr const char* jpAEnvD = "jpAEnvD";
    inline constexpr const char* jpAEnvS = "jpAEnvS";
    inline constexpr const char* jpAEnvR = "jpAEnvR";
}

namespace eighty
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using P  = AudioParameterFloat;
    using Pc = AudioParameterChoice;
    using Pb = AudioParameterBool;
    using Pi = AudioParameterInt;

    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto freqRange = NormalisableRange<float>(20.f, 18000.f, 0.f, 0.25f);
    auto timeRange = [] (float lo, float hi) { return NormalisableRange<float>(lo, hi, 0.f, 0.35f); };

    StringArray feet { "32'", "16'", "8'", "4'" };
    auto pct = NormalisableRange<float>(0.f, 1.f, 0.f);

    // Osc I
    p.push_back(std::make_unique<Pb>(ID::osc1On, "Osc I On", true));
    p.push_back(std::make_unique<Pc>(ID::osc1Foot, "Osc I Range", feet, 2));
    p.push_back(std::make_unique<P>(ID::osc1Fine, "Osc I Fine", NormalisableRange<float>(-50.f, 50.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::osc1Saw,   "Osc I Saw",   pct, 0.8f));
    p.push_back(std::make_unique<P>(ID::osc1Pulse, "Osc I Pulse", pct, 0.0f));
    p.push_back(std::make_unique<P>(ID::osc1PW,  "Osc I PW",  NormalisableRange<float>(0.05f, 0.95f, 0.f), 0.5f));
    p.push_back(std::make_unique<P>(ID::osc1PWM, "Osc I PWM", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::osc1Level, "Osc I Level", pct, 0.9f));

    // Osc II
    p.push_back(std::make_unique<Pb>(ID::osc2On, "Osc II On", true));
    p.push_back(std::make_unique<Pc>(ID::osc2Foot, "Osc II Range", feet, 2));
    p.push_back(std::make_unique<Pi>(ID::osc2Semi, "Osc II Semi", -12, 12, 0));
    p.push_back(std::make_unique<P>(ID::osc2Fine, "Osc II Fine", NormalisableRange<float>(-50.f, 50.f, 0.f), 6.f));
    p.push_back(std::make_unique<P>(ID::osc2Saw,   "Osc II Saw",   pct, 0.8f));
    p.push_back(std::make_unique<P>(ID::osc2Pulse, "Osc II Pulse", pct, 0.0f));
    p.push_back(std::make_unique<P>(ID::osc2PW,  "Osc II PW",  NormalisableRange<float>(0.05f, 0.95f, 0.f), 0.5f));
    p.push_back(std::make_unique<P>(ID::osc2PWM, "Osc II PWM", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::osc2Level, "Osc II Level", pct, 0.9f));

    p.push_back(std::make_unique<P>(ID::noiseLevel, "Noise", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::pwmRate, "PWM Rate",
        NormalisableRange<float>(0.05f, 20.f, 0.f, 0.4f), 0.6f));

    // Sub-oscillators
    StringArray subOcts { "-1", "-2" };
    StringArray subWaves { "Sqr", "Tri" };
    p.push_back(std::make_unique<P>(ID::csSubLevel, "CS Sub", pct, 0.f));
    p.push_back(std::make_unique<Pc>(ID::csSubOct, "CS Sub Octave", subOcts, 0));
    p.push_back(std::make_unique<Pc>(ID::csSubWave, "CS Sub Wave", subWaves, 0));
    p.push_back(std::make_unique<P>(ID::jpSubLevel, "JP Sub", pct, 0.f));
    p.push_back(std::make_unique<Pc>(ID::jpSubOct, "JP Sub Octave", subOcts, 0));
    p.push_back(std::make_unique<Pc>(ID::jpSubWave, "JP Sub Wave", subWaves, 0));

    p.push_back(std::make_unique<P>(ID::csSineLevel, "CS Sine", pct, 0.f));

    // Filter
    p.push_back(std::make_unique<P>(ID::hpfCutoff, "HPF Cutoff", freqRange, 20.f));
    p.push_back(std::make_unique<P>(ID::lpfCutoff, "LPF Cutoff", freqRange, 9000.f));
    p.push_back(std::make_unique<P>(ID::resonance, "Resonance", pct, 0.15f));
    p.push_back(std::make_unique<P>(ID::hpfRes, "HPF Res", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::filterEnvAmt, "Filter Env", NormalisableRange<float>(-1.f, 1.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::keyTrack, "Key Track", pct, 0.5f));
    p.push_back(std::make_unique<P>(ID::filterDrive, "Drive", pct, 0.2f));

    // CS-80 channel II offsets (default all-neutral, so a patch that does not
    // touch them sounds exactly as it did with one shared filter)
    p.push_back(std::make_unique<P>(ID::csCh2Cut, "Ch II Cutoff",
        NormalisableRange<float>(-3.f, 3.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::csCh2Res, "Ch II Res",
        NormalisableRange<float>(-1.f, 1.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::csCh2Env, "Ch II Env",
        NormalisableRange<float>(-1.f, 1.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::csCh2Time, "Ch II Time",
        NormalisableRange<float>(0.25f, 4.f, 0.f, 0.5f), 1.f));

    // Ring modulator
    p.push_back(std::make_unique<Pb>(ID::ringOn, "Ring On", false));
    p.push_back(std::make_unique<P>(ID::ringDepth, "Ring Depth", pct, 0.5f));
    p.push_back(std::make_unique<P>(ID::ringRate, "Ring Rate",
        NormalisableRange<float>(0.5f, 3000.f, 0.f, 0.25f), 220.f));
    p.push_back(std::make_unique<P>(ID::ringEnvAmt, "Ring Env",
        NormalisableRange<float>(-1.f, 1.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::ringAtk, "Ring Attack", timeRange(0.001f, 5.f), 0.01f));
    p.push_back(std::make_unique<P>(ID::ringDec, "Ring Decay",  timeRange(0.005f, 10.f), 0.5f));

    // Global performance macros + render quality
    p.push_back(std::make_unique<P>(ID::brilliance, "Brilliance",
        NormalisableRange<float>(-1.f, 1.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::resOffset, "Res Offset",
        NormalisableRange<float>(-1.f, 1.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::velBend, "Vel Bend",
        NormalisableRange<float>(0.f, 2.f, 0.f), 0.3f));
    p.push_back(std::make_unique<Pc>(ID::oversample, "Oversampling",
        StringArray { "Off", "2x", "4x", "8x" }, 1));

    // Envelopes
    p.push_back(std::make_unique<P>(ID::fEnvA, "F.Env Attack",  timeRange(0.001f, 10.f), 0.005f));
    p.push_back(std::make_unique<P>(ID::fEnvD, "F.Env Decay",   timeRange(0.005f, 10.f), 0.35f));
    p.push_back(std::make_unique<P>(ID::fEnvS, "F.Env Sustain", pct, 0.4f));
    p.push_back(std::make_unique<P>(ID::fEnvR, "F.Env Release", timeRange(0.005f, 10.f), 0.3f));
    p.push_back(std::make_unique<P>(ID::fEnvIL, "F.Env Initial", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::fEnvAL, "F.Env Peak",    pct, 1.f));
    p.push_back(std::make_unique<P>(ID::aEnvA, "A.Env Attack",  timeRange(0.001f, 10.f), 0.004f));
    p.push_back(std::make_unique<P>(ID::aEnvD, "A.Env Decay",   timeRange(0.005f, 10.f), 0.4f));
    p.push_back(std::make_unique<P>(ID::aEnvS, "A.Env Sustain", pct, 0.8f));
    p.push_back(std::make_unique<P>(ID::aEnvR, "A.Env Release", timeRange(0.005f, 10.f), 0.35f));
    p.push_back(std::make_unique<P>(ID::velToAmp,    "Vel > Amp",    pct, 0.5f));
    p.push_back(std::make_unique<P>(ID::velToFilter, "Vel > Filter", pct, 0.3f));

    // LFO
    p.push_back(std::make_unique<P>(ID::lfoRate, "LFO Rate",
        NormalisableRange<float>(0.02f, 20.f, 0.f, 0.4f), 4.5f));
    p.push_back(std::make_unique<Pc>(ID::lfoWave, "LFO Wave",
        StringArray { "Sine", "Triangle", "Square", "Saw", "S&H" }, 0));
    p.push_back(std::make_unique<P>(ID::lfoDelay, "LFO Delay", timeRange(0.f, 3.f), 0.f));
    p.push_back(std::make_unique<P>(ID::lfoToPitch,  "LFO > Pitch",  pct, 0.f));
    p.push_back(std::make_unique<P>(ID::lfoToFilter, "LFO > Filter", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::lfoToAmp,    "LFO > Amp",    pct, 0.f));

    // Touch
    p.push_back(std::make_unique<P>(ID::touchRise, "Touch Rise", timeRange(0.05f, 5.f), 0.6f));
    p.push_back(std::make_unique<P>(ID::touchToVib,    "Touch > Vib",    pct, 0.25f));
    p.push_back(std::make_unique<P>(ID::touchToBright, "Touch > Bright", pct, 0.2f));
    p.push_back(std::make_unique<P>(ID::touchToLevel,  "Touch > Level",  pct, 0.15f));

    // Voices (per-engine)
    StringArray voiceModes { "Poly", "Mono", "Legato", "Unison", "Stack" };
    StringArray glideModes { "Off", "Legato", "Always" };
    p.push_back(std::make_unique<Pc>(ID::csVoiceMode, "CS Voice Mode", voiceModes, 0));
    p.push_back(std::make_unique<Pi>(ID::csPolyVoices, "CS Voices", 1, 16, 8));
    p.push_back(std::make_unique<Pi>(ID::csUnisonCount, "CS Unison Voices", 2, 8, 4));
    p.push_back(std::make_unique<P>(ID::csUnisonDetune, "CS Unison Detune", pct, 0.25f));
    p.push_back(std::make_unique<Pc>(ID::jpVoiceMode, "JP Voice Mode", voiceModes, 0));
    p.push_back(std::make_unique<Pi>(ID::jpPolyVoices, "JP Voices", 1, 16, 8));
    p.push_back(std::make_unique<Pi>(ID::jpUnisonCount, "JP Unison Voices", 2, 8, 4));
    p.push_back(std::make_unique<P>(ID::jpUnisonDetune, "JP Unison Detune", pct, 0.25f));
    p.push_back(std::make_unique<P>(ID::stereoSpread, "Spread", pct, 0.6f));
    p.push_back(std::make_unique<P>(ID::drift, "Drift", pct, 0.35f));
    p.push_back(std::make_unique<P>(ID::csGlideTime, "CS Glide", timeRange(0.f, 5.f), 0.05f));
    p.push_back(std::make_unique<Pc>(ID::csGlideMode, "CS Glide Mode", glideModes, 0));
    p.push_back(std::make_unique<P>(ID::jpGlideTime, "JP Glide", timeRange(0.f, 5.f), 0.05f));
    p.push_back(std::make_unique<Pc>(ID::jpGlideMode, "JP Glide Mode", glideModes, 0));
    p.push_back(std::make_unique<Pi>(ID::bendRange, "Bend Range", 1, 24, 2));
    p.push_back(std::make_unique<Pb>(ID::hold, "Hold", false));

    // Tempo (drives the arp, the sequencer and the synced delay)
    p.push_back(std::make_unique<P>(ID::tempo, "Tempo",
        NormalisableRange<float>(20.f, 300.f, 0.1f), 120.f));

    // Arp
    p.push_back(std::make_unique<Pb>(ID::csArpOn, "CS Arp", false));
    p.push_back(std::make_unique<Pb>(ID::jpArpOn, "JP Arp", false));
    p.push_back(std::make_unique<Pc>(ID::arpMode, "Arp Mode",
        StringArray { "Up", "Down", "Up-Down", "Random", "As Played" }, 0));
    p.push_back(std::make_unique<Pb>(ID::arpSync, "Arp Sync", true));
    p.push_back(std::make_unique<Pc>(ID::arpDiv, "Arp Div",
        StringArray { "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 5));
    p.push_back(std::make_unique<Pi>(ID::arpOctaves, "Arp Octaves", 1, 4, 1));
    p.push_back(std::make_unique<Pc>(ID::arpMult, "Arp Rate",
        StringArray { "x1", "x2", "x3", "x4", "x6", "x8" }, 0));
    p.push_back(std::make_unique<P>(ID::arpGate, "Arp Gate", NormalisableRange<float>(0.05f, 1.f, 0.f), 0.6f));

    // Step sequencer (2 tracks)
    StringArray seqEngines { "Auto", "CS-80", "JP-8" };
    p.push_back(std::make_unique<Pb>(ID::seqRec,  "Seq Rec",  false));
    p.push_back(std::make_unique<Pb>(ID::seqPlay, "Seq Play", false));
    p.push_back(std::make_unique<Pc>(ID::seqTrack, "Seq Track",
        StringArray { "A", "B" }, 0));
    p.push_back(std::make_unique<Pb>(ID::seqAMute, "Seq A Mute", false));
    p.push_back(std::make_unique<Pb>(ID::seqBMute, "Seq B Mute", false));
    p.push_back(std::make_unique<Pc>(ID::seqAEng, "Seq A Engine", seqEngines, 0));
    p.push_back(std::make_unique<Pc>(ID::seqBEng, "Seq B Engine", seqEngines, 0));
    p.push_back(std::make_unique<Pi>(ID::seqALen, "Seq A Length", 1, 16, 16));
    p.push_back(std::make_unique<Pi>(ID::seqBLen, "Seq B Length", 1, 16, 16));

    // Trackpad performance (strum / ribbon)
    p.push_back(std::make_unique<Pb>(ID::tpOn, "Trackpad", true));
    p.push_back(std::make_unique<Pc>(ID::tpKey, "Strum Key",
        StringArray { "Q", "Space", "`" }, 0));
    p.push_back(std::make_unique<Pb>(ID::tpLatch, "Strum Latch", false));
    p.push_back(std::make_unique<Pc>(ID::tpTarget, "Strum Plays",
        StringArray { "Chord", "Pattern" }, 0));
    p.push_back(std::make_unique<Pi>(ID::tpSpan, "Strum Span", 1, 4, 2));
    p.push_back(std::make_unique<P>(ID::tpForce, "Strum Force", pct, 0.6f));
    p.push_back(std::make_unique<Pc>(ID::tpAxis, "Strum Axis",
        StringArray { "X", "Y" }, 0));
    p.push_back(std::make_unique<Pc>(ID::tpCross, "Cross Axis",
        StringArray { "Off", "Velocity", "Brilliance", "Pressure" }, 1));
    p.push_back(std::make_unique<Pc>(ID::tpArtic, "Strum Articulation",
        StringArray { "Hold", "Pluck" }, 0));
    p.push_back(std::make_unique<Pc>(ID::tpLift, "On Lift",
        StringArray { "Ring", "Damp", "Gesture" }, 2));
    p.push_back(std::make_unique<Pc>(ID::tpFingers, "Two Fingers",
        StringArray { "Per Engine", "Both Engines" }, 0));
    p.push_back(std::make_unique<Pb>(ID::tpFreeze, "Freeze Pointer", true));
    p.push_back(std::make_unique<Pb>(ID::tpMonitor, "Touch Monitor", false));
    p.push_back(std::make_unique<Pb>(ID::tpRibbon, "Ribbon", true));
    p.push_back(std::make_unique<Pi>(ID::tpRibbonRng, "Ribbon Range", 1, 24, 12));
    p.push_back(std::make_unique<Pb>(ID::tpRibbonAbs, "Ribbon Absolute", false));

    // FX
    p.push_back(std::make_unique<Pb>(ID::chorusOn, "Chorus On", true));
    p.push_back(std::make_unique<Pc>(ID::chorusMode, "Chorus Mode",
        StringArray { "I", "II", "Ens" }, 0));
    p.push_back(std::make_unique<P>(ID::chorusRate,  "Chorus Rate",  NormalisableRange<float>(0.05f, 5.f, 0.f, 0.5f), 0.6f));
    p.push_back(std::make_unique<P>(ID::chorusDepth, "Chorus Depth", pct, 0.35f));
    p.push_back(std::make_unique<P>(ID::chorusMix,   "Chorus Mix",   pct, 0.4f));
    p.push_back(std::make_unique<Pb>(ID::delayOn, "Delay On", false));
    p.push_back(std::make_unique<P>(ID::delayTime, "Delay Time", timeRange(0.02f, 2.f), 0.35f));
    p.push_back(std::make_unique<Pb>(ID::delaySync, "Delay Sync", true));
    p.push_back(std::make_unique<Pc>(ID::delayDiv, "Delay Div",
        StringArray { "1/1", "1/2", "1/4", "1/4.", "1/8", "1/8.", "1/8T", "1/16" }, 4));
    p.push_back(std::make_unique<P>(ID::delayFB,  "Delay FB",  NormalisableRange<float>(0.f, 0.95f, 0.f), 0.35f));
    p.push_back(std::make_unique<P>(ID::delayMix, "Delay Mix", pct, 0.25f));
    p.push_back(std::make_unique<Pb>(ID::tremOn, "Trem On", false));
    p.push_back(std::make_unique<P>(ID::tremRate,  "Trem Rate",  NormalisableRange<float>(0.1f, 12.f, 0.f, 0.5f), 4.f));
    p.push_back(std::make_unique<P>(ID::tremDepth, "Trem Depth", pct, 0.5f));

    // Master
    p.push_back(std::make_unique<P>(ID::masterVol, "Volume",
        NormalisableRange<float>(-36.f, 6.f, 0.f), -6.f));
    p.push_back(std::make_unique<P>(ID::masterTune, "Tune",
        NormalisableRange<float>(-100.f, 100.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::limitDrive, "Limiter",
        NormalisableRange<float>(0.f, 24.f, 0.f), 0.f));
    p.push_back(std::make_unique<P>(ID::stereoWidth, "Width",
        NormalisableRange<float>(0.f, 2.f, 0.f), 1.f));

    // Engine / split
    p.push_back(std::make_unique<Pc>(ID::engineMode, "Engine",
        StringArray { "CS-80", "JP-8", "Split", "Layer", "Keys" }, 0));
    p.push_back(std::make_unique<Pi>(ID::splitPoint, "Split Point", 36, 84, 60));
    p.push_back(std::make_unique<Pb>(ID::splitCsLow, "CS Low", true));
    p.push_back(std::make_unique<P>(ID::engineBalance, "CS/JP Mix", pct, 0.5f));

    // Output mixer
    p.push_back(std::make_unique<P>(ID::csLevel, "CS Level", pct, 1.f));
    p.push_back(std::make_unique<Pb>(ID::csMute, "CS Mute", false));
    p.push_back(std::make_unique<Pb>(ID::csSolo, "CS Solo", false));
    p.push_back(std::make_unique<P>(ID::jpLevel, "JP Level", pct, 1.f));
    p.push_back(std::make_unique<Pb>(ID::jpMute, "JP Mute", false));
    p.push_back(std::make_unique<Pb>(ID::jpSolo, "JP Solo", false));
    p.push_back(std::make_unique<P>(ID::synthLevel, "Synth Level", pct, 0.8f));
    p.push_back(std::make_unique<Pb>(ID::synthMute, "Synth Mute", false));
    p.push_back(std::make_unique<Pb>(ID::synthSolo, "Synth Solo", false));

    // Jupiter-8
    StringArray jpFeet { "16'", "8'", "4'", "2'" };
    p.push_back(std::make_unique<Pc>(ID::jpVco1Wave, "VCO1 Wave",
        StringArray { "Tri", "Saw", "Pulse", "Square" }, 1));
    p.push_back(std::make_unique<Pc>(ID::jpVco1Range, "VCO1 Range", jpFeet, 1));
    p.push_back(std::make_unique<P>(ID::jpPW,  "JP PW",  NormalisableRange<float>(0.05f, 0.95f, 0.f), 0.5f));
    p.push_back(std::make_unique<P>(ID::jpPWM, "JP PWM", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::jpMix, "VCO Mix", pct, 0.5f));
    p.push_back(std::make_unique<Pc>(ID::jpVco2Wave, "VCO2 Wave",
        StringArray { "Tri", "Saw", "Pulse", "Noise" }, 1));
    p.push_back(std::make_unique<Pc>(ID::jpVco2Range, "VCO2 Range", jpFeet, 1));
    p.push_back(std::make_unique<Pi>(ID::jpVco2Semi, "VCO2 Semi", -12, 12, 0));
    p.push_back(std::make_unique<P>(ID::jpVco2Fine, "VCO2 Fine", NormalisableRange<float>(-50.f, 50.f, 0.f), 5.f));
    p.push_back(std::make_unique<Pb>(ID::jpVco2Low, "VCO2 Low", false));
    p.push_back(std::make_unique<Pb>(ID::jpSync, "Sync", false));
    p.push_back(std::make_unique<P>(ID::jpXmod, "Cross Mod", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::jpHpf, "JP HPF",
        NormalisableRange<float>(20.f, 2000.f, 0.f, 0.3f), 20.f));
    p.push_back(std::make_unique<P>(ID::jpLpf, "JP LPF", freqRange, 9000.f));
    p.push_back(std::make_unique<P>(ID::jpRes, "JP Res", pct, 0.15f));
    p.push_back(std::make_unique<P>(ID::jpDrive, "JP Drive", pct, 0.15f));
    p.push_back(std::make_unique<Pb>(ID::jpSlope24, "24 dB", true));
    p.push_back(std::make_unique<P>(ID::jpEnvAmt, "JP Env Amt", NormalisableRange<float>(-1.f, 1.f, 0.f), 0.f));
    p.push_back(std::make_unique<Pb>(ID::jpEnvInv, "JP Env Invert", false));
    p.push_back(std::make_unique<P>(ID::jpKeyTrk, "JP Key Trk", pct, 0.5f));
    p.push_back(std::make_unique<P>(ID::jpFEnvA, "JP F.Attack",  timeRange(0.001f, 10.f), 0.005f));
    p.push_back(std::make_unique<P>(ID::jpFEnvD, "JP F.Decay",   timeRange(0.005f, 10.f), 0.35f));
    p.push_back(std::make_unique<P>(ID::jpFEnvS, "JP F.Sustain", pct, 0.4f));
    p.push_back(std::make_unique<P>(ID::jpFEnvR, "JP F.Release", timeRange(0.005f, 10.f), 0.3f));
    p.push_back(std::make_unique<P>(ID::jpAEnvA, "JP A.Attack",  timeRange(0.001f, 10.f), 0.004f));
    p.push_back(std::make_unique<P>(ID::jpAEnvD, "JP A.Decay",   timeRange(0.005f, 10.f), 0.4f));
    p.push_back(std::make_unique<P>(ID::jpAEnvS, "JP A.Sustain", pct, 0.8f));
    p.push_back(std::make_unique<P>(ID::jpAEnvR, "JP A.Release", timeRange(0.005f, 10.f), 0.35f));

    return { p.begin(), p.end() };
}
} // namespace eighty
