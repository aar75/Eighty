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

    // Filter (CS-80 topology: 12 dB/oct HPF into 12 dB/oct resonant LPF)
    inline constexpr const char* hpfCutoff     = "hpfCutoff";
    inline constexpr const char* lpfCutoff     = "lpfCutoff";
    inline constexpr const char* resonance     = "resonance";
    inline constexpr const char* filterEnvAmt  = "filterEnvAmt";
    inline constexpr const char* keyTrack      = "keyTrack";
    inline constexpr const char* filterDrive   = "filterDrive";

    // Envelopes
    inline constexpr const char* fEnvA = "fEnvA";
    inline constexpr const char* fEnvD = "fEnvD";
    inline constexpr const char* fEnvS = "fEnvS";
    inline constexpr const char* fEnvR = "fEnvR";
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

    // Voices
    inline constexpr const char* voiceMode    = "voiceMode";   // Poly / Mono / Legato / Unison
    inline constexpr const char* polyVoices   = "polyVoices";
    inline constexpr const char* unisonCount  = "unisonCount";
    inline constexpr const char* unisonDetune = "unisonDetune";
    inline constexpr const char* stereoSpread = "stereoSpread";
    inline constexpr const char* drift        = "drift";       // analog inconsistency amount
    inline constexpr const char* glideTime    = "glideTime";
    inline constexpr const char* glideMode    = "glideMode";   // Off / Legato / Always
    inline constexpr const char* bendRange    = "bendRange";
    inline constexpr const char* hold         = "hold";

    // Arpeggiator
    inline constexpr const char* arpOn      = "arpOn";
    inline constexpr const char* arpMode    = "arpMode";
    inline constexpr const char* arpSync    = "arpSync";
    inline constexpr const char* arpRateHz  = "arpRateHz";
    inline constexpr const char* arpDiv     = "arpDiv";
    inline constexpr const char* arpOctaves = "arpOctaves";
    inline constexpr const char* arpGate    = "arpGate";

    // FX
    inline constexpr const char* chorusOn    = "chorusOn";
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

    // Engine selection / keyboard split
    inline constexpr const char* engineMode = "engineMode";   // CS-80 / JP-8 / Split
    inline constexpr const char* splitPoint = "splitPoint";
    inline constexpr const char* splitCsLow = "splitCsLow";   // CS on the low side?

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
    inline constexpr const char* jpSync      = "jpSync";
    inline constexpr const char* jpXmod      = "jpXmod";
    inline constexpr const char* jpHpf       = "jpHpf";
    inline constexpr const char* jpLpf       = "jpLpf";
    inline constexpr const char* jpRes       = "jpRes";
    inline constexpr const char* jpSlope24   = "jpSlope24";
    inline constexpr const char* jpEnvAmt    = "jpEnvAmt";
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

    // Filter
    p.push_back(std::make_unique<P>(ID::hpfCutoff, "HPF Cutoff", freqRange, 20.f));
    p.push_back(std::make_unique<P>(ID::lpfCutoff, "LPF Cutoff", freqRange, 9000.f));
    p.push_back(std::make_unique<P>(ID::resonance, "Resonance", pct, 0.15f));
    p.push_back(std::make_unique<P>(ID::filterEnvAmt, "Filter Env", NormalisableRange<float>(-1.f, 1.f, 0.f), 0.35f));
    p.push_back(std::make_unique<P>(ID::keyTrack, "Key Track", pct, 0.5f));
    p.push_back(std::make_unique<P>(ID::filterDrive, "Drive", pct, 0.2f));

    // Envelopes
    p.push_back(std::make_unique<P>(ID::fEnvA, "F.Env Attack",  timeRange(0.001f, 10.f), 0.005f));
    p.push_back(std::make_unique<P>(ID::fEnvD, "F.Env Decay",   timeRange(0.005f, 10.f), 0.35f));
    p.push_back(std::make_unique<P>(ID::fEnvS, "F.Env Sustain", pct, 0.4f));
    p.push_back(std::make_unique<P>(ID::fEnvR, "F.Env Release", timeRange(0.005f, 10.f), 0.3f));
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

    // Voices
    p.push_back(std::make_unique<Pc>(ID::voiceMode, "Voice Mode",
        StringArray { "Poly", "Mono", "Legato", "Unison" }, 0));
    p.push_back(std::make_unique<Pi>(ID::polyVoices, "Voices", 1, 16, 8));
    p.push_back(std::make_unique<Pi>(ID::unisonCount, "Unison Voices", 2, 8, 4));
    p.push_back(std::make_unique<P>(ID::unisonDetune, "Unison Detune", pct, 0.25f));
    p.push_back(std::make_unique<P>(ID::stereoSpread, "Spread", pct, 0.6f));
    p.push_back(std::make_unique<P>(ID::drift, "Drift", pct, 0.35f));
    p.push_back(std::make_unique<P>(ID::glideTime, "Glide", timeRange(0.f, 5.f), 0.05f));
    p.push_back(std::make_unique<Pc>(ID::glideMode, "Glide Mode",
        StringArray { "Off", "Legato", "Always" }, 0));
    p.push_back(std::make_unique<Pi>(ID::bendRange, "Bend Range", 1, 24, 2));
    p.push_back(std::make_unique<Pb>(ID::hold, "Hold", false));

    // Arp
    p.push_back(std::make_unique<Pb>(ID::arpOn, "Arp On", false));
    p.push_back(std::make_unique<Pc>(ID::arpMode, "Arp Mode",
        StringArray { "Up", "Down", "Up-Down", "Random", "As Played" }, 0));
    p.push_back(std::make_unique<Pb>(ID::arpSync, "Arp Sync", true));
    p.push_back(std::make_unique<P>(ID::arpRateHz, "Arp Rate",
        NormalisableRange<float>(0.5f, 20.f, 0.f, 0.5f), 5.f));
    p.push_back(std::make_unique<Pc>(ID::arpDiv, "Arp Div",
        StringArray { "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 5));
    p.push_back(std::make_unique<Pi>(ID::arpOctaves, "Arp Octaves", 1, 4, 1));
    p.push_back(std::make_unique<P>(ID::arpGate, "Arp Gate", NormalisableRange<float>(0.05f, 1.f, 0.f), 0.6f));

    // FX
    p.push_back(std::make_unique<Pb>(ID::chorusOn, "Chorus On", true));
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

    // Engine / split
    p.push_back(std::make_unique<Pc>(ID::engineMode, "Engine",
        StringArray { "CS-80", "JP-8", "Split", "Layer" }, 0));
    p.push_back(std::make_unique<Pi>(ID::splitPoint, "Split Point", 36, 84, 60));
    p.push_back(std::make_unique<Pb>(ID::splitCsLow, "CS Low", true));

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
    p.push_back(std::make_unique<Pb>(ID::jpSync, "Sync", false));
    p.push_back(std::make_unique<P>(ID::jpXmod, "Cross Mod", pct, 0.f));
    p.push_back(std::make_unique<P>(ID::jpHpf, "JP HPF",
        NormalisableRange<float>(20.f, 2000.f, 0.f, 0.3f), 20.f));
    p.push_back(std::make_unique<P>(ID::jpLpf, "JP LPF", freqRange, 9000.f));
    p.push_back(std::make_unique<P>(ID::jpRes, "JP Res", pct, 0.15f));
    p.push_back(std::make_unique<Pb>(ID::jpSlope24, "24 dB", true));
    p.push_back(std::make_unique<P>(ID::jpEnvAmt, "JP Env Amt", NormalisableRange<float>(-1.f, 1.f, 0.f), 0.35f));
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
