#include "PluginEditor.h"
#include <map>
#include <algorithm>
#include <limits>

namespace
{
    const juce::String& plexSansName()
    {
        static const juce::String name =
            juce::Font::findAllTypefaceNames().contains ("IBM Plex Sans")
                ? juce::String ("IBM Plex Sans") : juce::String();
        return name;
    }
    const juce::String& plexMonoName()
    {
        static const juce::String name =
            juce::Font::findAllTypefaceNames().contains ("IBM Plex Mono")
                ? juce::String ("IBM Plex Mono") : juce::String ("Menlo");
        return name;
    }
}

namespace ui
{
    juce::Font sans (float size, bool bold)
    {
        auto o = juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain);
        if (plexSansName().isNotEmpty())
            o = o.withName (plexSansName());
        return juce::Font (o);
    }
    juce::Font mono (float size)
    {
        return juce::Font (juce::FontOptions().withName (plexMonoName()).withHeight (size));
    }
}

// ============================================================== tooltips
namespace
{
juce::String tipFor (const juce::String& id)
{
    static const std::map<juce::String, const char*> tips = {
        { ID::osc1On,    "Enable oscillator channel I" },
        { ID::osc1Foot,  "Octave range of channel I (32' lowest, 4' highest)" },
        { ID::osc1Fine,  "Fine tune channel I, in cents" },
        { ID::osc1Saw,   "Sawtooth level, channel I" },
        { ID::osc1Pulse, "Pulse level, channel I" },
        { ID::osc1PW,    "Pulse width, channel I (50% = square)" },
        { ID::osc1PWM,   "Pulse-width modulation depth from the PWM LFO, channel I" },
        { ID::osc1Level, "Output level of channel I" },
        { ID::osc2On,    "Enable oscillator channel II" },
        { ID::osc2Foot,  "Octave range of channel II (32' lowest, 4' highest)" },
        { ID::osc2Semi,  "Transpose channel II, in semitones" },
        { ID::osc2Fine,  "Fine tune channel II, in cents (slight offset = classic beating)" },
        { ID::osc2Saw,   "Sawtooth level, channel II" },
        { ID::osc2Pulse, "Pulse level, channel II" },
        { ID::osc2PW,    "Pulse width, channel II (50% = square)" },
        { ID::osc2PWM,   "Pulse-width modulation depth from the PWM LFO, channel II" },
        { ID::osc2Level, "Output level of channel II" },
        { ID::noiseLevel,"White noise level (CS-80 engine only)" },
        { ID::pwmRate,   "Speed of the shared PWM LFO (both engines)" },
        { ID::csSubLevel,"Sub-oscillator level: weight under the CS-80 oscillators, into the filter" },
        { ID::csSubOct,  "How far below Osc I the sub sits: one or two octaves" },
        { ID::csSubWave, "Sub waveform: square (hollow, Juno-ish) or triangle (rounder, less harmonic)" },
        { ID::jpSubLevel,"Sub-oscillator level: weight under the JP-8 VCOs, into the filter" },
        { ID::jpSubOct,  "How far below VCO1 the sub sits: one or two octaves" },
        { ID::jpSubWave, "Sub waveform: square (hollow, Juno-ish) or triangle (rounder, less harmonic)" },
        { ID::csSineLevel, "Sine level. Routed around the filters straight into the mixer, "
                           "as on the CS-80 - keeps a fundamental under a closed filter" },
        { ID::hpfCutoff,   "High-pass cutoff: removes lows below this frequency (12 dB/oct)" },
        { ID::lpfCutoff,   "Low-pass cutoff: removes highs above this frequency (12 dB/oct)" },
        { ID::resonance,   "Resonance peak at the low-pass cutoff" },
        { ID::hpfRes,      "Resonance on the high-pass too, as the CS-80 has. Open the HPF "
                           "against a closed LPF for its bandpass, chime and vocal patches" },
        { ID::filterEnvAmt,"Filter envelope sweep amount (negative inverts the sweep)" },
        { ID::keyTrack,    "Cutoff follows keyboard position" },
        { ID::filterDrive, "Input saturation, for analog warmth" },
        { ID::csCh2Cut,  "Channel II cutoff offset, in octaves. Channel II is a complete "
                         "second synth with its own filter - offsetting it is what makes "
                         "CS-80 brass and strings move" },
        { ID::csCh2Res,  "Channel II resonance offset" },
        { ID::csCh2Env,  "Channel II filter envelope amount, offset from channel I" },
        { ID::csCh2Time, "Channel II envelope speed, as a ratio of channel I. Above 1 makes "
                         "the second layer swell in behind the first" },
        { ID::ringOn,     "Ring modulator on" },
        { ID::ringDepth,  "Ring modulator depth: dry at 0, full multiplication at 1" },
        { ID::ringRate,   "Carrier frequency. Deliberately not keyboard-tracked - that fixed "
                          "pitch against the note is where the metallic character comes from" },
        { ID::ringEnvAmt, "How far the ring envelope pushes the carrier's *speed* - the detail "
                          "that makes it sound like the original rather than a multiplier" },
        { ID::ringAtk,    "Ring envelope attack time" },
        { ID::ringDec,    "Ring envelope decay time" },
        { ID::brilliance, "Global cutoff offset over every filter in the instrument. The most "
                          "useful control on a real CS-80" },
        { ID::resOffset,  "Global resonance offset over every filter in the instrument" },
        { ID::velBend,    "Touch Response initial pitch bend: a hard key strike slides up into "
                          "the note from below, the way a brass player's embouchure does" },
        { ID::oversample, "Render quality. The engine runs at this multiple of the sample rate, "
                          "which keeps filter drive, hard sync and cross-mod from folding their "
                          "own harmonics back down. Higher costs proportionally more CPU" },
        { ID::fEnvIL, "Initial level: where the filter envelope starts from, rather than 0. "
                      "Raising it shortens the sweep and dulls the tone (CS-80 IL)" },
        { ID::fEnvA, "Filter envelope attack time" },
        { ID::fEnvD, "Filter envelope decay time" },
        { ID::fEnvS, "Filter envelope sustain level" },
        { ID::fEnvR, "Filter envelope release time" },
        { ID::fEnvAL, "Attack level: the peak the attack segment reaches (CS-80 AL)" },
        { ID::aEnvA, "Amp envelope attack time" },
        { ID::aEnvD, "Amp envelope decay time" },
        { ID::aEnvS, "Amp envelope sustain level" },
        { ID::aEnvR, "Amp envelope release time" },
        { ID::velToFilter, "How much key velocity opens the filter" },
        { ID::velToAmp,    "How much key velocity controls loudness" },
        { ID::lfoRate,    "LFO speed" },
        { ID::lfoWave,    "LFO shape" },
        { ID::lfoDelay,   "LFO fade-in time after the first key press" },
        { ID::lfoToPitch, "Vibrato: LFO to pitch" },
        { ID::lfoToFilter,"Filter wobble: LFO to cutoff" },
        { ID::lfoToAmp,   "Tremolo: LFO to voice level" },
        { ID::touchRise,    "How quickly simulated key pressure builds while a key is held" },
        { ID::touchToVib,   "Pressure adds vibrato (real aftertouch works too)" },
        { ID::touchToBright,"Pressure opens the filter" },
        { ID::touchToLevel, "Pressure raises the volume" },
        { ID::csVoiceMode,   "CS-80: Poly plays chords; Mono retriggers; Legato doesn't; Unison stacks a fixed "
                             "number of voices on one note; Stack spreads the whole voice pool over whatever "
                             "you hold (16 cards on one note, 8 each on two...)" },
        { ID::csPolyVoices,  "CS-80: maximum simultaneous voices (also the size of the pool Stack divides up)" },
        { ID::csUnisonCount, "CS-80: voices stacked per note in Unison mode" },
        { ID::csUnisonDetune,"CS-80: detune spread across the unison / stack voices" },
        { ID::jpVoiceMode,   "JP-8: Poly plays chords; Mono retriggers; Legato doesn't; Unison stacks a fixed "
                             "number of voices on one note; Stack spreads the whole voice pool over whatever "
                             "you hold (16 cards on one note, 8 each on two...)" },
        { ID::jpPolyVoices,  "JP-8: maximum simultaneous voices (also the size of the pool Stack divides up)" },
        { ID::jpUnisonCount, "JP-8: voices stacked per note in Unison mode" },
        { ID::jpUnisonDetune,"JP-8: detune spread across the unison / stack voices" },
        { ID::stereoSpread,"Voice panning width: odd/even voice cards go left/right" },
        { ID::drift,       "Analog inconsistency: per-voice tuning, cutoff, envelope and pan tolerances" },
        { ID::csGlideTime, "CS-80 portamento time between notes" },
        { ID::csGlideMode, "CS-80 glide: off, only on overlapping notes, or always" },
        { ID::jpGlideTime, "JP-8 portamento time between notes" },
        { ID::jpGlideMode, "JP-8 glide: off, only on overlapping notes, or always" },
        { ID::bendRange,   "Pitch wheel range, in semitones" },
        { ID::hold,        "Latch notes after release (shortcut: B). A new chord replaces the held one" },
        { ID::csArpOn,   "Arpeggiate the CS-80 card. Per engine, so the CS can walk a chord "
                         "while the JP-8 holds it underneath (shortcut: N toggles both)" },
        { ID::jpArpOn,   "Arpeggiate the JP-8 card. Per engine, so the JP can walk a chord "
                         "while the CS-80 holds it underneath (shortcut: N toggles both)" },
        { ID::seqRec,    "Step record into the armed track from the cursor: play a note or chord, and the "
                         "cursor moves on when you let go. Stops after one lap of the loop" },
        { ID::seqPlay,   "Run both sequencer tracks. You can keep playing live on top of them" },
        { ID::seqTrack,  "Which track REC writes to, and which the CLEAR / COPY buttons act on" },
        { ID::seqAMute,  "Silence track A (it keeps running, so it stays in sync)" },
        { ID::seqBMute,  "Silence track B (it keeps running, so it stays in sync)" },
        { ID::seqAEng,   "Which voice card track A plays. Auto follows the engine mode / key map" },
        { ID::seqBEng,   "Which voice card track B plays. Auto follows the engine mode / key map" },
        { ID::seqALen,   "Loop length of track A. Different lengths per track give polymeter" },
        { ID::seqBLen,   "Loop length of track B. Different lengths per track give polymeter" },
        { ID::arpMode,   "Note order. 'PLY' plays your held notes in order, like a step sequencer" },
        { ID::arpSync,   "Follow the host's tempo instead of the panel TEMPO. A standalone build "
                         "has no host tempo, so TEMPO keeps working there either way" },
        { ID::tempo,     "Tempo for the arpeggiator, the sequencer and the synced delay, in BPM. "
                         "The step length is this against the DIV note division" },
        { ID::arpDiv,    "Note division: how long one arp or sequencer step is against the tempo" },
        { ID::arpOctaves,"Repeat the pattern across extra octaves" },
        { ID::arpMult,   "Arp notes per DIV step. x1 is the step clock itself; above that the "
                         "arp subdivides it - which is how an arpeggio runs inside a sequenced "
                         "chord instead of only changing when the chord does" },
        { ID::arpGate,   "Note length within each step" },
        { ID::chorusOn,    "Bucket-brigade style stereo chorus" },
        { ID::chorusMode,  "I: wide and slow. II: faster and deeper. ENS: three taps, the "
                           "thicker CS-80 ensemble rather than a chorus" },
        { ID::chorusRate,  "Chorus sweep speed" },
        { ID::chorusDepth, "Chorus sweep depth" },
        { ID::chorusMix,   "Chorus wet amount" },
        { ID::delayOn,     "Stereo delay with cross-feedback" },
        { ID::delayTime,   "Delay time, used when Sync is off" },
        { ID::delaySync,   "Sync the delay time to the host tempo" },
        { ID::delayDiv,    "Delay note division, used when Sync is on" },
        { ID::delayFB,     "Delay feedback (repeats)" },
        { ID::delayMix,    "Delay wet amount" },
        { ID::tremOn,      "Output tremolo with slight stereo offset" },
        { ID::tremRate,    "Tremolo speed" },
        { ID::tremDepth,   "Tremolo depth" },
        { ID::masterVol,  "Output volume" },
        { ID::masterTune, "Global tune, in cents" },
        { ID::limitDrive, "Output limiter: pushes the signal into a ceiling just under 0 dB. "
                          "At 0 it is a transparent safety; turn up for loudness and squash" },
        { ID::stereoWidth,"Stereo width of the output: 0 collapses to mono, 1 is neutral, 2 is double-wide. "
                          "Watch the vector display in the header" },
        { ID::engineMode, "What sounds: one engine everywhere, Split by key, Layer both on every note, "
                          "or Keys: paint each key's engine on the strip above the keyboard" },
        { ID::splitPoint, "Split key: notes below and above route to different engines (Split mode)" },
        { ID::splitCsLow, "CS-80 takes the low side of the split (off = JP-8 low)" },
        { ID::engineBalance, "Balance the CS-80 and JP-8 engines (Split & Layer modes). Center = both full" },
        { ID::csLevel,    "Output level of the CS-80 card" },
        { ID::csMute,     "Silence the CS-80 card" },
        { ID::csSolo,     "Solo the CS-80 card: everything not soloed goes quiet" },
        { ID::jpLevel,    "Output level of the JP-8 card" },
        { ID::jpMute,     "Silence the JP-8 card" },
        { ID::jpSolo,     "Solo the JP-8 card: everything not soloed goes quiet" },
        { ID::synthLevel, "Level of the hosted VST3 synth layer" },
        { ID::synthMute,  "Silence the hosted VST3 synth layer" },
        { ID::synthSolo,  "Solo the synth layer: everything not soloed goes quiet" },
        { ID::jpVco1Wave,  "VCO1 waveform" },
        { ID::jpVco1Range, "VCO1 octave range (16' lowest, 2' highest)" },
        { ID::jpPW,        "Pulse width for both VCOs (50% = square)" },
        { ID::jpPWM,       "Pulse-width modulation depth from the PWM LFO" },
        { ID::jpMix,       "Balance: VCO1 (bottom) to VCO2 (top)" },
        { ID::jpVco2Wave,  "VCO2 waveform (Noise replaces the oscillator)" },
        { ID::jpVco2Range, "VCO2 octave range (16' lowest, 2' highest)" },
        { ID::jpVco2Semi,  "Transpose VCO2, in semitones" },
        { ID::jpVco2Fine,  "Fine tune VCO2, in cents" },
        { ID::jpVco2Low,   "LOW: VCO2 stops tracking the keyboard and runs at LF, becoming a "
                           "per-voice modulator for sync and cross-mod" },
        { ID::jpSync,      "Hard-sync VCO2 to VCO1: VCO2 restarts each VCO1 cycle" },
        { ID::jpXmod,      "Cross-mod: VCO2 frequency-modulates VCO1" },
        { ID::jpHpf,       "Non-resonant high-pass cutoff (6 dB/oct, as on the JP-8)" },
        { ID::jpLpf,       "Low-pass cutoff" },
        { ID::jpRes,       "Ladder resonance. 24 dB mode self-oscillates at the top; 12 dB "
                           "resonates but cannot, exactly like the real slope switch" },
        { ID::jpDrive,     "Ladder input drive: the OTA's soft clipping" },
        { ID::jpSlope24,   "24 dB/oct filter slope, tapped from stage 4 (off = 12 dB/oct, "
                           "tapped from stage 2 - what the IR3109 actually does)" },
        { ID::jpEnvAmt,    "Filter envelope sweep amount (negative inverts)" },
        { ID::jpEnvInv,    "ENV-1 polarity: inverts the filter envelope for reverse sweeps" },
        { ID::jpKeyTrk,    "Cutoff follows keyboard position" },
        { ID::jpFEnvA, "Filter envelope attack time" },
        { ID::jpFEnvD, "Filter envelope decay time" },
        { ID::jpFEnvS, "Filter envelope sustain level" },
        { ID::jpFEnvR, "Filter envelope release time" },
        { ID::jpAEnvA, "Amp envelope attack time" },
        { ID::jpAEnvD, "Amp envelope decay time" },
        { ID::jpAEnvS, "Amp envelope sustain level" },
        { ID::jpAEnvR, "Amp envelope release time" },
    };
    auto it = tips.find (id);
    return it != tips.end() ? juce::String (it->second) : juce::String();
}

// compact value text for the persistent per-control readouts
juce::String shortValueText (const juce::String& id, double val)
{
    const float v = (float) val;
    auto in = [&id] (std::initializer_list<const char*> ids)
    {
        for (auto* s : ids)
            if (id == s) return true;
        return false;
    };

    if (in ({ ID::hpfCutoff, ID::lpfCutoff, ID::jpHpf, ID::jpLpf }))
        return v >= 1000.f ? juce::String (v / 1000.f, 1) + "k"
                           : juce::String ((int) std::round (v)) + "Hz";

    if (id == ID::ringRate)
        return v >= 1000.f ? juce::String (v / 1000.f, 1) + "k"
                           : juce::String (v, v < 10.f ? 1 : 0) + "Hz";

    if (in ({ ID::lfoRate, ID::pwmRate, ID::chorusRate, ID::tremRate }))
        return juce::String (v, 1) + "Hz";

    if (id == ID::tempo) return juce::String (v, 1);

    if (in ({ ID::fEnvA, ID::fEnvD, ID::fEnvR, ID::aEnvA, ID::aEnvD, ID::aEnvR,
              ID::jpFEnvA, ID::jpFEnvD, ID::jpFEnvR, ID::jpAEnvA, ID::jpAEnvD, ID::jpAEnvR,
              ID::ringAtk, ID::ringDec,
              ID::lfoDelay, ID::touchRise, ID::csGlideTime, ID::jpGlideTime, ID::delayTime }))
    {
        if (v < 0.0995f) return juce::String ((int) std::round (v * 1000.f)) + "ms";
        if (v < 0.9995f) return juce::String ((int) std::round (v * 1000.f)) + "m";
        return juce::String (v, 1) + "s";
    }

    if (in ({ ID::osc1PW, ID::osc2PW, ID::jpPW, ID::osc1PWM, ID::osc2PWM, ID::jpPWM,
              ID::keyTrack, ID::jpKeyTrk, ID::velToFilter, ID::velToAmp, ID::engineBalance }))
        return juce::String ((int) std::round (v * 100.f)) + "%";

    if (in ({ ID::osc1Fine, ID::osc2Fine, ID::jpVco2Fine, ID::masterTune }))
        return (v > 0.5f ? "+" : "") + juce::String ((int) std::round (v)) + "c";

    if (in ({ ID::osc2Semi, ID::jpVco2Semi }))
        return (v > 0.5f ? "+" : "") + juce::String ((int) std::round (v));

    if (id == ID::masterVol)  return juce::String (v, 1) + "dB";
    if (id == ID::limitDrive) return juce::String ((int) std::round (v)) + "dB";
    if (id == ID::splitPoint) return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 3);
    if (id == ID::bendRange)  return juce::String ((int) std::round (v)) + "st";

    if (in ({ ID::filterEnvAmt, ID::jpEnvAmt, ID::csCh2Res, ID::csCh2Env,
              ID::ringEnvAmt, ID::brilliance, ID::resOffset }))
        return (v >= 0.005f ? "+" : "") + juce::String (v, 2);

    if (id == ID::csCh2Cut)  return (v >= 0.005f ? "+" : "") + juce::String (v, 1) + "oct";
    if (id == ID::csCh2Time) return juce::String (v, 2) + "x";
    if (id == ID::velBend)   return juce::String (v, 2) + "st";

    if (in ({ ID::csPolyVoices, ID::csUnisonCount, ID::jpPolyVoices, ID::jpUnisonCount,
              ID::arpOctaves, ID::seqALen, ID::seqBLen }))
        return juce::String ((int) std::round (v));

    if (in ({ ID::stereoWidth, ID::csLevel, ID::jpLevel, ID::synthLevel }))
        return juce::String ((int) std::round (v * 100.f)) + "%";

    return juce::String (v, 2);
}

// ======================================================== scale detection
// Names the most common scale that contains the notes in the sequencer.
// Every root x family is scored and the best wins, compared in this order:
//
//   1. how much of the pattern the scale actually contains (weighted by how
//      often each pitch class is played) - a scale that misses a note is
//      always worse than one that does not;
//   2. how tightly it fits: a five-note pentatonic beats the major scale
//      that also happens to contain the same five notes;
//   3. whether the root is the note the pattern leans on. This is what
//      settles the relative-major ambiguity - the same seven notes read as
//      C major or A minor, and which one you meant is in the bass;
//   4. how common the family is, for anything still tied.
struct ScaleFamily { const char* name; int size; int steps[7]; };

static const ScaleFamily kFamilies[] = {
    { "MAJOR",       7, { 0, 2, 4, 5, 7, 9, 11 } },
    { "MINOR",       7, { 0, 2, 3, 5, 7, 8, 10 } },
    { "DORIAN",      7, { 0, 2, 3, 5, 7, 9, 10 } },
    { "MIXOLYDIAN",  7, { 0, 2, 4, 5, 7, 9, 10 } },
    { "LYDIAN",      7, { 0, 2, 4, 6, 7, 9, 11 } },
    { "PHRYGIAN",    7, { 0, 1, 3, 5, 7, 8, 10 } },
    { "LOCRIAN",     7, { 0, 1, 3, 5, 6, 8, 10 } },
    { "HARM MINOR",  7, { 0, 2, 3, 5, 7, 8, 11 } },
    { "MEL MINOR",   7, { 0, 2, 3, 5, 7, 9, 11 } },
    { "MAJ PENT",    5, { 0, 2, 4, 7, 9, 0, 0 } },
    { "MIN PENT",    5, { 0, 3, 5, 7, 10, 0, 0 } },
    { "BLUES",       6, { 0, 3, 5, 6, 7, 10, 0 } },
    { "WHOLE TONE",  6, { 0, 2, 4, 6, 8, 10, 0 } },
};

struct ScaleFit
{
    juce::String name;      // empty when there is nothing to name
    bool exact = true;      // false if the pattern spills outside the scale
    int distinct = 0;       // distinct pitch classes in the pattern
    int covered = 0;        // how many of them the named scale contains
};

// weights[pc] = how often that pitch class is played; strongest / lowest are
// the pitch classes of the most-played and the lowest-sounding note.
ScaleFit analyseScale (const int (&weights)[12], int strongestPc, int lowestPc)
{
    ScaleFit fit;
    for (int pc = 0; pc < 12; ++pc)
        if (weights[pc] > 0) ++fit.distinct;

    // One pitch class is a note, not a key - naming a scale off it would be
    // an invention rather than a reading.
    if (fit.distinct < 2) return fit;

    int best[4] = { std::numeric_limits<int>::max(), 0, 0, 0 };
    for (int root = 0; root < 12; ++root)
    {
        const int rootRank = root == strongestPc ? (root == lowestPc ? 0 : 1)
                                                 : (root == lowestPc ? 2 : 3);
        for (int fi = 0; fi < (int) (sizeof (kFamilies) / sizeof (kFamilies[0])); ++fi)
        {
            const auto& fam = kFamilies[fi];
            bool inScale[12] = {};
            for (int s = 0; s < fam.size; ++s)
                inScale[(root + fam.steps[s]) % 12] = true;

            int missing = 0, covered = 0;
            for (int pc = 0; pc < 12; ++pc)
                if (weights[pc] > 0)
                {
                    if (inScale[pc]) ++covered;
                    else             missing += weights[pc];
                }

            const int score[4] = { missing, fam.size, rootRank, fi };
            if (std::lexicographical_compare (score, score + 4, best, best + 4))
            {
                std::copy (score, score + 4, best);
                static const char* names[] = { "C", "C#", "D", "D#", "E", "F",
                                               "F#", "G", "G#", "A", "A#", "B" };
                fit.name = juce::String (names[root]) + " " + fam.name;
                fit.exact = missing == 0;
                fit.covered = covered;
            }
        }
    }
    return fit;
}
} // namespace

// ============================================================ LookAndFeel
EightyLNF::EightyLNF()
{
    setColour (juce::ResizableWindow::backgroundColourId, ui::winBg);
    setColour (juce::Label::textColourId, ui::ink);
    setColour (juce::PopupMenu::backgroundColourId, ui::cardBg);
    setColour (juce::PopupMenu::textColourId, ui::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::ctrlLine);
    setColour (juce::PopupMenu::highlightedTextColourId, ui::textHi);
    setColour (juce::TooltipWindow::backgroundColourId, ui::scopeBg);
    setColour (juce::TooltipWindow::textColourId, ui::ink);
    setColour (juce::TooltipWindow::outlineColourId, ui::line);
    setColour (juce::TextButton::buttonColourId, ui::ctrlBg);
    setColour (juce::TextButton::textColourOffId, ui::inkSoft);
    setColour (juce::TextButton::textColourOnId, ui::textHi);
    setColour (juce::TextButton::buttonOnColourId, ui::ctrlLine);
    setColour (juce::AlertWindow::backgroundColourId, ui::cardBg);
    setColour (juce::AlertWindow::textColourId, ui::ink);
    // Keys stay light against the dark chassis - a dark-on-dark keyboard is
    // unplayable, and the mockup keeps them bright too.
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffcdd5e3));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff2b3444));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, ui::keyDown);
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ui::keyDown.withAlpha (0.45f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff232b3b));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
}

void EightyLNF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float minSliderPos, float maxSliderPos,
                                 juce::Slider::SliderStyle, juce::Slider& slider)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float trackW = 7.f;
    auto accent = slider.findColour (juce::Slider::trackColourId);
    if (accent.isTransparent()) accent = ui::scopeTrace;

    // recessed groove
    g.setColour (ui::groove);
    g.fillRoundedRectangle (cx - trackW * 0.5f, (float) y, trackW, (float) h, 3.5f);
    g.setColour (ui::ctrlLine);
    g.drawRoundedRectangle (cx - trackW * 0.5f + 0.5f, (float) y + 0.5f,
                            trackW - 1.f, (float) h - 1.f, 3.f, 1.f);

    // Accent fill from the fader's "home" up to the cap: from the bottom for
    // a unipolar control, from the centre for a bipolar one, so a negative
    // Filter Env reads as a bar hanging *below* zero rather than a nearly
    // empty track.
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float zeroY = bipolar
        ? minSliderPos + (float) slider.valueToProportionOfLength (0.0) * (maxSliderPos - minSliderPos)
        : (float) (y + h);
    {
        const float top = juce::jmin (zeroY, pos), bot = juce::jmax (zeroY, pos);
        g.setColour (accent.withAlpha (0.8f));
        g.fillRoundedRectangle (cx - trackW * 0.5f + 1.f, top,
                                trackW - 2.f, juce::jmax (1.f, bot - top), 2.5f);
    }

    // Zero tick for bipolar faders - marks where "off"/centred actually is,
    // since the bottom of the fader is the minimum, not zero.
    if (bipolar)
    {
        g.setColour (ui::dim.withAlpha (0.8f));
        g.fillRect (cx - 7.f, zeroY - 0.5f, 14.f, 1.f);
    }

    // cap: light slate with an accent line across it
    const float capW = 22.f, capH = 13.f;
    const float cy = juce::jlimit ((float) y, (float) (y + h) - capH, pos - capH * 0.5f);
    auto cap = juce::Rectangle<float> (cx - capW * 0.5f, cy, capW, capH);
    g.setGradientFill (juce::ColourGradient (ui::thumbHi, cap.getX(), cap.getY(),
                                             ui::thumbLo, cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.f);
    g.setColour (ui::thumbEdge);
    g.drawRoundedRectangle (cap.reduced (0.5f), 3.f, 1.f);
    g.setColour (accent);
    g.fillRect (cap.getCentreX() - 7.f, cap.getCentreY() - 1.f, 14.f, 2.f);
}

void EightyLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float startAngle, float endAngle,
                                 juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto sq = bounds.withSizeKeepingCentre (size, size);
    const auto c = sq.getCentre();
    // Face and arc are sized as fractions of the knob, not by fixed insets,
    // so the 28 px header knobs show the same clear ring as the 48 px
    // section knobs instead of hiding the arc under their own face.
    const float r = size * 0.44f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    auto accent = slider.findColour (juce::Slider::trackColourId);
    if (accent.isTransparent()) accent = ui::scopeTrace;

    // value arc: accent up to the value, groove colour for the rest
    if (endAngle - angle > 0.01f)
    {
        juce::Path rest;
        rest.addCentredArc (c.x, c.y, r, r, 0.f, angle, endAngle, true);
        g.setColour (ui::groove);
        g.strokePath (rest, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
    if (angle - startAngle > 0.01f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r, r, 0.f, startAngle, angle, true);
        // faint bloom behind the arc, so the value reads at a glance
        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (arc, juce::PathStrokeType (6.f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (arc, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    auto face = sq.reduced (size * 0.166f);
    auto faceCol = slider.findColour (juce::Slider::rotarySliderFillColourId);
    g.setColour (faceCol.isTransparent() ? ui::knobFace : faceCol);
    g.fillEllipse (face);
    g.setColour (ui::knobRim);
    g.drawEllipse (face.reduced (0.75f), 1.5f);

    juce::Path p;
    p.addRoundedRectangle (-1.5f, -size * 0.5f + size * 0.17f, 3.f, size * 0.33f, 1.5f);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (ui::ink);
    g.fillPath (p);

    g.setColour (ui::knobHub);
    g.fillEllipse (juce::Rectangle<float> (size * 0.26f, size * 0.26f).withCentre (c));
}

void EightyLNF::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                     const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool active = down || b.getToggleState();

    // A button given its own "on" colour keeps it while it is on. The
    // recorder needs to read as armed from across the room, which one step
    // lighter than the panel grey does not do.
    const auto onCol = b.isColourSpecified (juce::TextButton::buttonOnColourId)
                     ? b.findColour (juce::TextButton::buttonOnColourId) : ui::ctrlLine;

    g.setColour (active ? onCol : (over ? ui::ctrlBg.brighter (0.12f) : ui::ctrlBg));
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (active ? onCol.brighter (0.35f) : (over ? ui::knobRim : ui::ctrlLine));
    g.drawRoundedRectangle (r, 4.f, 1.f);
}

juce::Font EightyLNF::getTextButtonFont (juce::TextButton&, int) { return ui::mono (10.f); }
juce::Font EightyLNF::getPopupMenuFont() { return ui::sans (13.f); }

// ========================================================== fader / knob
VFader::VFader (const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, ui::inkSoft);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
    value.setJustificationType (juce::Justification::centred);
    value.setFont (ui::mono (8.5f));
    value.setColour (juce::Label::textColourId, ui::scopeTrace);
    value.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (value);
}

void VFader::paint (juce::Graphics& g)
{
    auto box = value.getBounds().toFloat().reduced (0.5f, 0.5f);
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (box, 2.5f);
    g.setColour (ui::track);
    g.drawRoundedRectangle (box, 2.5f, 1.f);
}

void VFader::resized()
{
    const float fs = label.getText().length() >= 5 ? 9.f
                   : (getWidth() < 28 ? 9.5f : 10.5f);
    label.setFont (ui::sans (fs, true));
    auto b = getLocalBounds();
    value.setBounds (b.removeFromBottom (12));
    label.setBounds (b.removeFromBottom (14));
    slider.setBounds (b);
}

MiniKnob::MiniKnob (const juce::String& text, bool headerStyle) : header (headerStyle)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setWantsKeyboardFocus (false);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, ui::knobFace);
    addAndMakeVisible (slider);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (ui::sans (header ? 8.5f : 10.f, true));
    label.setColour (juce::Label::textColourId, header ? ui::inkSoft : ui::mid);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
    value.setJustificationType (juce::Justification::centred);
    value.setFont (ui::mono (header ? 9.f : 8.5f));
    value.setColour (juce::Label::textColourId, ui::scopeTrace);
    value.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (value);
}

void MiniKnob::paint (juce::Graphics& g)
{
    auto box = value.getBounds().toFloat().reduced (header ? 1.f : 3.f, 0.5f);
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (box, 2.5f);
    g.setColour (ui::track);
    g.drawRoundedRectangle (box, 2.5f, 1.f);
}

void MiniKnob::resized()
{
    const int kd = knobSize > 0 ? knobSize : getWidth() - 6;
    // narrow slots get a smaller caption rather than an ellipsis
    if (! header)
        label.setFont (ui::sans (getWidth() >= 40 ? 10.f : (getWidth() >= 34 ? 9.f : 8.f), true));
    if (header)
    {
        slider.setBounds ((getWidth() - kd) / 2, 0, kd, kd);
        label.setBounds (0, kd + 1, getWidth(), 11);
        value.setBounds (0, kd + 12, getWidth(), 10);
        return;
    }
    const int block = kd + 3 + 13 + 12;
    const int top = juce::jmax (0, (getHeight() - block) / 2 + 7);
    slider.setBounds ((getWidth() - kd) / 2, top, kd, kd);
    label.setBounds (0, top + kd + 3, getWidth(), 13);
    value.setBounds (0, top + kd + 16, getWidth(), 12);
}

// ============================================================== ChipStack
ChipStack::ChipStack (juce::RangedAudioParameter& p, juce::StringArray l,
                      const juce::String& g, bool horizontal, juce::Colour onColour)
    : param (p),
      att (p, [this] (float v) { selected = (int) std::round (v); repaint(); }),
      labels (std::move (l)), group (g), horiz (horizontal), onCol (onColour)
{
    paramID = p.paramID;
    att.sendInitialUpdate();
}

void ChipStack::paint (juce::Graphics& g)
{
    const int n = labels.size();
    if (n == 0) return;

    // A selected chip is an accent-tinted panel with an accent border and
    // bright text, rather than a solid accent block: at this size a solid
    // fill of a saturated colour swamps the label.
    auto drawChip = [&g] (juce::Rectangle<float> r, const juce::String& text,
                          bool on, juce::Colour accent, float fontSize, float corner)
    {
        g.setColour (on ? accent.withAlpha (0.28f) : ui::chipOff);
        g.fillRoundedRectangle (r, corner);
        g.setColour (on ? accent : ui::track);
        g.drawRoundedRectangle (r, corner, 1.f);
        g.setColour (on ? ui::textHi : ui::dim);
        g.setFont (ui::sans (fontSize, true));
        g.drawText (text, r, juce::Justification::centred);
    };

    if (horiz)
    {
        for (int i = 0; i < n; ++i)
        {
            auto r = juce::Rectangle<int> (i * cellW, 0, cellW, getHeight()).toFloat().reduced (1.f, 0.5f);
            // engine-named chips carry their engine's identity colour
            auto fill = labels[i].contains ("JP") ? ui::jpAccent
                      : labels[i].contains ("CS") ? ui::stOsc : onCol;
            drawChip (r, labels[i], i == selected, fill, cellW < 40 ? 9.f : 10.f, 3.5f);
        }
        return;
    }

    // vertical: chip stack, caption at the bottom
    const int ch = chipH();
    const int top = getHeight() - 14 - (n * (ch - 1) + 1);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<int> (0, top + i * (ch - 1), getWidth(), ch)
                     .toFloat().reduced (0.5f, 1.f);
        drawChip (r, labels[i], i == selected, onCol, 9.5f, 2.5f);
    }
    g.setColour (ui::dim);
    g.setFont (ui::sans (9.f, true));
    g.drawText (group, 0, getHeight() - 11, getWidth(), 11, juce::Justification::centred);
}

void ChipStack::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (onRightClick) onRightClick (e.getScreenPosition());
        return;
    }
    const int n = labels.size();
    if (n == 0) return;
    int idx;
    if (horiz)
        idx = juce::jlimit (0, n - 1, e.x / juce::jmax (1, cellW));
    else
    {
        const int ch = chipH();
        const int top = getHeight() - 14 - (n * (ch - 1) + 1);
        idx = juce::jlimit (0, n - 1, (e.y - top) / (ch - 1));
    }
    att.setValueAsCompleteGesture ((float) idx);
    if (onUserChange) onUserChange();
}

// ============================================================== LedToggle
LedToggle::LedToggle (juce::RangedAudioParameter& p, const juce::String& t)
    : param (p),
      att (p, [this] (float v) { on = v > 0.5f; repaint(); }),
      text (t)
{
    paramID = p.paramID;
    att.sendInitialUpdate();
}

int LedToggle::preferredWidth() const
{
    // lens + gap + text + the pill's own padding, with a pixel of slack so
    // the label never ellipsises against its own box. Single-letter pills
    // (the mixer's M/S) drop the trailing padding - three strips of them
    // have to fit inside one 58 px mixer column.
    const int textW = (int) std::ceil (ui::sans (9.5f, true).getStringWidthFloat (text));
    return (text.length() <= 1 ? 17 : 21) + textW;
}

void LedToggle::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    auto pill = b.toFloat().reduced (0.5f);
    g.setColour (on ? ui::ledOn.withAlpha (0.16f) : ui::ctrlBg);
    g.fillRoundedRectangle (pill, 3.5f);
    g.setColour (on ? ui::ledOn.withAlpha (0.7f) : ui::track);
    g.drawRoundedRectangle (pill, 3.5f, 1.f);

    auto led = juce::Rectangle<float> (4.f, (float) b.getCentreY() - 3.5f, 7.f, 7.f);
    if (on)   // glow ring under the lens
    {
        g.setColour (ui::ledOn.withAlpha (0.3f));
        g.fillEllipse (led.expanded (3.f));
    }
    g.setColour (on ? ui::ledOn : ui::ledOff);
    g.fillEllipse (led);

    g.setColour (on ? ui::textHi : ui::inkSoft);
    g.setFont (ui::sans (9.5f, true));
    g.drawText (text, b.withTrimmedLeft (14).withTrimmedRight (3),
                juce::Justification::centredLeft);
}

void LedToggle::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (onRightClick) onRightClick (e.getScreenPosition());
        return;
    }
    att.setValueAsCompleteGesture (on ? 0.f : 1.f);
    if (onUserChange) onUserChange();
}

// ============================================================ MiniDisplay
MiniDisplay::MiniDisplay (Kind k, std::vector<juce::RangedAudioParameter*> p)
    : kind (k), params (std::move (p))
{
    // hover for the tooltip, but never swallow a click
    setInterceptsMouseClicks (true, false);
    setTooltip (kind == filterKind
        ? "Filter response: level against frequency, 20 Hz to 18 kHz on a log axis, "
          "with the resonance peak at each corner"
        : kind == adsrKind
        ? "Envelope shape. Segment widths are the real times, compressed so a long "
          "release still leaves the attack visible"
        : "LFO shape - two cycles of the selected waveform");
    cache.assign (params.size(), -1.f);
    startTimerHz (10);
}

void MiniDisplay::timerCallback()
{
    bool changed = false;
    for (size_t i = 0; i < params.size(); ++i)
    {
        const float v = params[i] != nullptr ? params[i]->getValue() : 0.f;
        if (std::abs (v - cache[i]) > 0.001f) { cache[i] = v; changed = true; }
    }
    if (changed) repaint();
}

float MiniDisplay::value (size_t i, float fallback) const
{
    if (i >= params.size() || params[i] == nullptr) return fallback;
    return params[i]->convertFrom0to1 (params[i]->getValue());
}

// One-pole-per-slope magnitude model: a 4-pole low pass, a 4-pole high pass
// and a resonant bump at each corner. Not either real filter's transfer
// function - it is a picture of the shape they make, which is what a
// 24-pixel-tall display can honestly show.
float MiniDisplay::filterMagDb (float f, float lpHz, float hpHz, float q, float qHp)
{
    auto pole4 = [] (float ratio)
    { return 10.f * std::log10 (1.f / (1.f + std::pow (ratio, 4.f))); };

    float db = pole4 (f / juce::jmax (1.f, lpHz)) + pole4 (juce::jmax (1.f, hpHz) / f);
    auto bump = [f] (float corner, float amount, float gain)
    {
        const float l = std::log (f / juce::jmax (1.f, corner));
        return amount * gain * std::exp (-l * l * 5.f);
    };
    db += bump (lpHz, q, 16.f);
    if (qHp > 0.f) db += bump (hpHz, qHp, 14.f);
    return db;
}

void MiniDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (ui::track);
    g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);

    // Everything below is laid out in a 100 x 28 space and scaled to fit, so
    // one set of numbers describes every display size.
    constexpr float W = 100.f, H = 28.f;
    juce::Path p, grid;

    if (kind == filterKind)
    {
        // hpf, lpf, res [, hpfRes] - swept over 20 Hz .. 18 kHz, log x
        const float hp = value (0, 20.f), lp = value (1, 18000.f);
        const float q = value (2), qHp = value (3);
        constexpr float loHz = 20.f, decades = 900.f;   // 20 Hz -> 18 kHz
        constexpr float topDb = 14.f, spanDb = 46.f;
        auto toY = [] (float db) { return (topDb - db) / spanDb * (H - 4.f) + 2.f; };

        grid.startNewSubPath (0.f, toY (0.f));
        grid.lineTo (W, toY (0.f));

        for (int i = 0; i <= 60; ++i)
        {
            const float x = (float) i / 60.f;
            const float f = loHz * std::pow (decades, x);
            const float db = juce::jlimit (-32.f, topDb, filterMagDb (f, lp, hp, q, qHp));
            const float py = toY (db);
            if (i == 0) p.startNewSubPath (0.f, py);
            else        p.lineTo (x * W, py);
        }
    }
    else if (kind == adsrKind)
    {
        // a, d, s, r [, initial level, attack level], times in seconds.
        // Segment widths are the real times raised to 0.4 so a 10 s release
        // does not squash the attack into a single pixel.
        auto tw = [this] (size_t i)
        { return 0.06f + std::pow (juce::jlimit (0.f, 1.f, value (i) / 10.f), 0.4f) * 0.55f; };

        const float sus = juce::jlimit (0.f, 1.f, value (2));
        const float il  = params.size() > 4 ? juce::jlimit (0.f, 1.f, value (4)) : 0.f;
        const float al  = params.size() > 5 ? juce::jlimit (0.f, 1.f, value (5, 1.f)) : 1.f;

        float wa = tw (0), wd = tw (1), wr = tw (3);
        const float total = wa + wd + wr + 0.3f;    // 0.3 = fixed sustain leg
        wa = wa / total * W; wd = wd / total * W; wr = wr / total * W;

        auto toY = [] (float v) { return H - 2.f - (H - 6.f) * v; };
        const float susY = toY (sus * al);

        grid.startNewSubPath (0.f, H - 2.f);
        grid.lineTo (W, H - 2.f);

        p.startNewSubPath (0.f, toY (il));
        p.lineTo (wa, toY (al));
        // decay and release curve rather than ramp - an exponential segment
        // is what the envelope actually does
        p.quadraticTo (wa + wd * 0.35f, susY, wa + wd, susY);
        p.lineTo (W - wr, susY);
        p.quadraticTo (W - wr * 0.6f, toY (0.f), W, toY (0.f));
    }
    else // lfoKind - two cycles of the selected shape
    {
        const int wave = params.empty() || params[0] == nullptr
                       ? 0 : (int) std::round (params[0]->getValue() * 4.f);
        constexpr float mid = H * 0.5f, amp = H * 0.5f - 3.f;
        constexpr int steps = 80;

        grid.startNewSubPath (0.f, mid);
        grid.lineTo (W, mid);

        static const float sh[8] = { 0.3f, -0.5f, 0.8f, -0.2f, 0.55f, -0.75f, 0.1f, -0.4f };
        float prev = 0.f;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float) i / (float) steps;
            const float ph = std::fmod (t * 2.f, 1.f);
            const float x = t * W;
            float v;
            switch (wave)
            {
                case 1:  v = 1.f - 4.f * std::abs (std::fmod (ph + 0.25f, 1.f) - 0.5f); break;
                case 2:  v = ph < 0.5f ? 1.f : -1.f; break;
                case 3:  v = 1.f - 2.f * ph; break;
                case 4:  v = sh[juce::jlimit (0, 7, (int) (t * 8.f))]; break;
                default: v = std::sin (t * juce::MathConstants<float>::twoPi * 2.f); break;
            }
            if (i == 0) p.startNewSubPath (x, mid - v * amp);
            else
            {
                // square and S&H step vertically instead of ramping
                if ((wave == 2 || wave == 4) && std::abs (v - prev) > 1.0e-6f)
                    p.lineTo (x, mid - prev * amp);
                p.lineTo (x, mid - v * amp);
            }
            prev = v;
        }
    }

    const auto scale = juce::AffineTransform::scale ((float) getWidth() / W,
                                                     (float) getHeight() / H);
    p.applyTransform (scale);
    grid.applyTransform (scale);

    g.setColour (ui::scopeLine);
    g.strokePath (grid, juce::PathStrokeType (1.f));
    // faint bloom, then the line itself
    g.setColour (trace.withAlpha (0.25f));
    g.strokePath (p, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
    g.setColour (trace);
    g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

// ================================================================ Section
Section::Section (const juce::String& n, juce::Colour s) : name (n), stripe (s) {}

void Section::paint (juce::Graphics& g)
{
    // Rounded card rather than the old hairline rule: against a dark
    // chassis a section needs its own surface to separate from the page.
    auto card = getLocalBounds().toFloat().reduced (2.f, 1.f);
    g.setColour (ui::cardBg);
    g.fillRoundedRectangle (card, 7.f);
    g.setColour (ui::line);
    g.drawRoundedRectangle (card.reduced (0.5f), 7.f, 1.f);

    auto f = ui::sans (11.f, true);
    f.setExtraKerningFactor (0.05f);
    g.setColour (ui::textHi);
    g.setFont (f);
    g.drawText (name, 14, 7, getWidth() - 20, 12, juce::Justification::centredLeft);
    g.setColour (stripe);
    g.fillRoundedRectangle (14.f, 21.f, 26.f, 3.f, 1.5f);
}

void Section::addItem (juce::Component& c, int width)
{
    items.push_back ({ &c, width });
    addAndMakeVisible (c);
}

void Section::addHeaderToggle (LedToggle& t)
{
    headerToggles.push_back (&t);
    addAndMakeVisible (t);
}

void Section::setDisplay (MiniDisplay* d, int width)
{
    display = d;
    displayW = width;
    addAndMakeVisible (d);
}

int Section::preferredWidth() const
{
    int w = 14 + 12;    // left border+pad, right pad
    for (size_t i = 0; i < items.size(); ++i)
        w += items[i].width + (i > 0 ? 6 : 0);
    return w;
}

void Section::resized()
{
    // LED toggles (or mini display) at top-right
    int tx = getWidth() - 12;
    for (auto it = headerToggles.rbegin(); it != headerToggles.rend(); ++it)
    {
        const int w = (*it)->preferredWidth();
        tx -= w;
        (*it)->setBounds (tx, 5, w, 16);
        tx -= 6;
    }
    if (display != nullptr)
        display->setBounds (getWidth() - 12 - displayW, 4, displayW, 24);

    auto b = getLocalBounds().withTrimmedTop (30).withTrimmedLeft (14)
                             .withTrimmedRight (12).withTrimmedBottom (8);
    int x = b.getX();
    for (auto& it : items)
    {
        it.comp->setBounds (x, b.getY(), it.width, b.getHeight());
        x += it.width + 6;
    }
}

// ================================================================== Scope
ScopeTap::ScopeTap (ScopeFifo& f) : fifo (f)
{
    l.resize (kHistory, 0.f);
    r.resize (kHistory, 0.f);
    startTimerHz (30);
}

void ScopeTap::timerCallback()
{
    float tl[2048], tr[2048];
    int n;
    while ((n = fifo.pull (tl, tr, 2048)) > 0)
        for (int i = 0; i < n; ++i)
        {
            l[(size_t) writePos] = tl[i];
            r[(size_t) writePos] = tr[i];
            writePos = (writePos + 1) & kMask;
        }
    for (auto* v : views)
        if (v->isShowing()) v->repaint();
}

ScopeComponent::ScopeComponent (ScopeTap& t) : tap (t)
{
    // hover (for the tooltip) but never swallow a click
    setInterceptsMouseClicks (true, false);
    setTooltip ("Output waveform, triggered on a rising zero crossing so the trace "
                "stands still. Graticule lines are quarter divisions");
    tap.addView (this);
}

void ScopeComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (r, 5.f);
    g.setColour (ui::track);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.f, 1.f);

    auto area = r.reduced (5.f, 5.f);

    // graticule: centre line plus quarter divisions each way
    {
        juce::Path grid;
        const float midY = area.getCentreY();
        grid.startNewSubPath (area.getX(), midY);
        grid.lineTo (area.getRight(), midY);
        for (int i = 1; i < 4; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / 4.f;
            grid.startNewSubPath (x, area.getY());
            grid.lineTo (x, area.getBottom());
        }
        for (int s = -1; s <= 1; s += 2)
        {
            const float y = midY + (float) s * area.getHeight() * 0.25f;
            grid.startNewSubPath (area.getX(), y);
            grid.lineTo (area.getRight(), y);
        }
        g.setColour (ui::scopeLine);
        g.strokePath (grid, juce::PathStrokeType (1.f));
    }

    // mono sum, rising-edge triggered so the trace stands still
    const auto& buf = tap.lBuf();
    const auto& bufR = tap.rBuf();
    auto sample = [&] (int idx)
    {
        const int i = idx & ScopeTap::kMask;
        return 0.5f * (buf[(size_t) i] + bufR[(size_t) i]);
    };

    constexpr int windowLen = 1400, points = 512;
    const int start = tap.pos() - windowLen - 512;
    int trig = start;
    for (int i = 0; i < 500; ++i)
        if (sample (start + i) <= 0.f && sample (start + i + 1) > 0.f) { trig = start + i + 1; break; }

    juce::Path p;
    const float midY = area.getCentreY();
    const float scaleY = area.getHeight() * 0.48f;
    for (int i = 0; i < points; ++i)
    {
        const float px = area.getX() + area.getWidth() * (float) i / (float) (points - 1);
        const float py = midY - juce::jlimit (-1.f, 1.f,
                            sample (trig + i * windowLen / points)) * scaleY;
        if (i == 0) p.startNewSubPath (px, py);
        else        p.lineTo (px, py);
    }

    // Translucent body between the trace and the centre line, so a quiet
    // patch still reads as a shape rather than a flat wire.
    {
        juce::Path body (p);
        body.lineTo (area.getRight(), midY);
        body.lineTo (area.getX(), midY);
        body.closeSubPath();
        g.setColour (ui::scopeTrace.withAlpha (0.13f));
        g.fillPath (body);
    }

    // Three passes of falling width fake the phosphor bloom the mockup gets
    // from a canvas shadow; JUCE has no cheap blur here.
    g.setColour (ui::scopeTrace.withAlpha (0.14f));
    g.strokePath (p, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
    g.setColour (ui::scopeTrace.withAlpha (0.3f));
    g.strokePath (p, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
    g.setColour (ui::scopeTrace);
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

// ============================================================== Lissajous
LissajousScope::LissajousScope (ScopeTap& t) : tap (t)
{
    // hover (for the tooltip) but never swallow a click
    setInterceptsMouseClicks (true, false);
    setTooltip ("Vector display: left against right, rotated so mono draws a vertical line. "
                "A wide, round shape is a wide stereo image; a horizontal line is out of phase. "
                "The trace fades over about a second, so the shape a patch traces stays readable");
    tap.addView (this);
}

void LissajousScope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float rad = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 5.f;

    // Mid/side rotation: y = L+R (mono energy, up), x = L-R (the difference),
    // zoomed so a loud wide patch fills the circle instead of sitting in a
    // dot at the middle. Overshoot is compressed *radially*, not per axis,
    // so a hot signal rounds off against the graticule rather than squaring
    // off into the corners.
    constexpr int frames = 1200;
    constexpr float zoom = 2.4f;
    auto softLimit = [] (float r)
    {
        constexpr float knee = 0.8f;
        return r <= knee ? r : knee + (1.f - knee) * std::tanh ((r - knee) / (1.f - knee));
    };

    juce::Path p;
    bool started = false;
    for (int i = frames; i > 0; --i)
    {
        const float L = tap.left (i), R = tap.right (i);
        float dx = (L - R) * 0.7071f * zoom;
        float dy = (L + R) * 0.7071f * zoom;
        const float r = std::sqrt (dx * dx + dy * dy);
        if (r > 1.0e-6f)
        {
            const float k = softLimit (r) / r;
            dx *= k;
            dy *= k;
        }
        const float x = c.x + dx * rad;
        const float y = c.y - dy * rad;
        if (! started) { p.startNewSubPath (x, y); started = true; }
        else           p.lineTo (x, y);
    }

    // Persistence layer. Rather than clearing, each frame lays a translucent
    // wash of the background over the last one and draws on top, so older
    // sweeps decay over roughly a dozen frames. The graticule is redrawn
    // every frame so it does not fade with them.
    const int iw = juce::jmax (1, getWidth()), ih = juce::jmax (1, getHeight());
    if (! phosphor.isValid() || phosphor.getWidth() != iw || phosphor.getHeight() != ih)
    {
        phosphor = juce::Image (juce::Image::ARGB, iw, ih, true);
        juce::Graphics ig (phosphor);
        ig.setColour (ui::scopeBg);
        ig.fillAll();
    }

    {
        juce::Graphics ig (phosphor);
        // Decay has to outrun the draw or a steady patch saturates to a
        // solid blob within a second and stops reading as a trace.
        ig.setColour (ui::scopeBg.withAlpha (0.4f));
        ig.fillAll();

        ig.setColour (ui::scopeTrace.withAlpha (0.16f));
        ig.strokePath (p, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
        ig.setColour (ui::scopeTrace.withAlpha (0.62f));
        ig.strokePath (p, juce::PathStrokeType (1.f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // rounded window: clip the decayed image to the panel shape
    {
        juce::Path clip;
        clip.addRoundedRectangle (b, 5.f);
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (clip);
        g.drawImageAt (phosphor, 0, 0);
    }

    // graticule: the two axes plus a bounding circle, over the trace
    g.setColour (ui::scopeLine.withAlpha (0.9f));
    g.drawEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f, 1.f);
    g.drawLine (c.x, c.y - rad, c.x, c.y + rad, 1.f);
    g.drawLine (c.x - rad, c.y, c.x + rad, c.y, 1.f);

    g.setColour (ui::track);
    g.drawRoundedRectangle (b.reduced (0.5f), 5.f, 1.f);
}

// ============================================================= PitchWheel
void PitchWheel::paint (juce::Graphics& g)
{
    auto body = getLocalBounds().toFloat().withTrimmedBottom (13.f);
    body = body.withSizeKeepingCentre (28.f, body.getHeight());
    // shaded barrel: dark at the ends, lit across the middle
    g.setGradientFill ([&]
    {
        juce::ColourGradient grad (ui::groove, body.getX(), body.getY(),
                                   ui::groove, body.getX(), body.getBottom(), false);
        grad.addColour (0.5, ui::wheelBg.brighter (0.15f));
        return grad;
    }());
    g.fillRoundedRectangle (body, 6.f);
    g.setColour (ui::ctrlLine);
    g.drawRoundedRectangle (body.reduced (0.5f), 6.f, 1.f);
    g.setColour (ui::knobRim.withAlpha (0.7f));   // centre detent
    g.fillRect (body.getX(), body.getCentreY() - 0.5f, body.getWidth(), 1.f);

    const float travel = body.getHeight() * 0.5f - 12.f;
    const float hy = body.getCentreY() - value * travel;
    auto handle = juce::Rectangle<float> (body.getX() + 3.f, hy - 8.f, body.getWidth() - 6.f, 16.f);
    g.setGradientFill (juce::ColourGradient (ui::thumbHi, handle.getX(), handle.getY(),
                                             ui::thumbLo, handle.getX(), handle.getBottom(), false));
    g.fillRoundedRectangle (handle, 3.f);
    g.setColour (ui::thumbEdge);
    g.drawRoundedRectangle (handle.reduced (0.5f), 3.f, 1.f);

    g.setColour (ui::dim);
    g.setFont (ui::sans (8.5f, true));
    g.drawText ("BEND", getLocalBounds().removeFromBottom (12), juce::Justification::centred);
}

void PitchWheel::setFromMouse (const juce::MouseEvent& e)
{
    const float half = (float) (getHeight() - 13) * 0.5f;
    value = juce::jlimit (-1.f, 1.f, (half - (float) e.y) / juce::jmax (1.f, half - 14.f));
    if (onChange) onChange (value, true);
    repaint();
}

void PitchWheel::mouseDown (const juce::MouseEvent& e) { setFromMouse (e); }
void PitchWheel::mouseDrag (const juce::MouseEvent& e) { setFromMouse (e); }
void PitchWheel::mouseUp (const juce::MouseEvent&)
{
    value = 0.f;
    if (onChange) onChange (0.f, false);
    repaint();
}

void PitchWheel::setExternalValue (float v)
{
    if (std::abs (v - value) > 0.001f) { value = v; repaint(); }
}

// ============================================================= PanelChips
void PanelChips::paint (juce::Graphics& g)
{
    const char* names[2] = { "CS-80 PANEL", "JP-8 PANEL" };
    for (int i = 0; i < 2; ++i)
    {
        auto r = juce::Rectangle<float> ((float) (i * (tabW + tabGap)), 0.5f,
                                         (float) tabW, (float) getHeight());
        const bool on = (i == 1) == jpSel;
        const auto col = i == 1 ? ui::jpAccent : ui::stOsc;

        juce::Path tab;
        tab.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                 4.f, 4.f, true, true, false, false);
        g.setColour (on ? col.withAlpha (0.28f) : ui::chipOff);
        g.fillPath (tab);
        g.setColour (on ? col : ui::track);
        g.strokePath (tab, juce::PathStrokeType (1.f));

        auto f = ui::sans (11.f, true);
        f.setExtraKerningFactor (0.06f);
        const float tw = f.getStringWidthFloat (names[i]);
        const float cx = r.getCentreX() + 7.f;              // shift right for the dot
        auto dot = juce::Rectangle<float> (cx - tw * 0.5f - 14.f, r.getCentreY() - 3.5f, 7.f, 7.f);
        if (on)                                             // lit tab: the dot glows
        {
            g.setColour (col.withAlpha (0.35f));
            g.fillEllipse (dot.expanded (3.f));
        }
        g.setColour (on ? col : ui::ledOff);
        g.fillEllipse (dot);
        g.setColour (on ? ui::textHi : ui::dim);
        g.setFont (f);
        g.drawText (names[i], (int) (cx - tw * 0.5f), 0, (int) tw + 4, getHeight(),
                    juce::Justification::centredLeft);
    }
}

void PanelChips::mouseDown (const juce::MouseEvent& e)
{
    if (onSelect) onSelect (e.x > tabW + tabGap / 2);
}

// =========================================================== KeyZoneStrip
KeyZoneStrip::KeyZoneStrip (EightyProcessor& p, juce::MidiKeyboardComponent& keyboard)
    : proc (p), kb (keyboard)
{
    setMode (2);
}

void KeyZoneStrip::setMode (int engineMode)
{
    mode = engineMode;
    setTooltip (mode == 4
        ? "Which engine each key triggers. Click: CS-80 > JP-8 > BOTH. Drag paints. Right-click: fill menu"
        : "Key zones. Click or drag to move the split point");
    repaint();
}

void KeyZoneStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::groove);
    g.fillRoundedRectangle (r, 3.f);

    const int splitNote = (int) proc.apvts.getRawParameterValue (ID::splitPoint)->load();
    const bool csLow    = proc.apvts.getRawParameterValue (ID::splitCsLow)->load() > 0.5f;

    for (int n = kb.getRangeStart(); n <= kb.getRangeEnd(); ++n)
    {
        auto kr = kb.getRectangleForKey (n);
        if (kr.getRight() < 0.f || kr.getX() > (float) getWidth())
            continue;

        int zone;
        if (mode == 2)
            zone = ((n < splitNote) == csLow) ? 0 : 1;
        else
            zone = proc.getKeyZone (n);

        // 0 = CS-80, 1 = JP-8, 2 = both
        const auto col = zone == 0 ? ui::stOsc : zone == 1 ? ui::jpAccent : ui::stEnv;
        g.setColour (col.withAlpha (0.72f));
        g.fillRect (kr.getX() + 0.5f, 2.f, kr.getWidth() - 1.f, (float) getHeight() - 4.f);
    }

    if (mode == 2)   // split point marker
    {
        const float x = kb.getRectangleForKey (splitNote).getX();
        if (x >= 0.f && x <= (float) getWidth())
        {
            g.setColour (ui::ledOn);
            g.fillRect (x - 1.f, 0.f, 2.f, (float) getHeight());
        }
    }
}

int KeyZoneStrip::noteAt (juce::Point<float> p) const
{
    return kb.getNoteAndVelocityAtPosition ({ p.x, 3.f }).note;
}

void KeyZoneStrip::applyPaint (int note)
{
    if (note < 0) return;
    if (mode == 2)
    {
        if (auto* p = proc.apvts.getParameter (ID::splitPoint))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) note));
            p->endChangeGesture();
        }
    }
    else
        proc.setKeyZone (note, paintValue);
    repaint();
}

void KeyZoneStrip::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (mode == 4) showFillMenu();
        return;
    }
    const int note = noteAt (e.position);
    if (note < 0) return;
    paintValue = (uint8_t) ((proc.getKeyZone (note) + 1) % 3);   // cycle on click
    applyPaint (note);
}

void KeyZoneStrip::mouseDrag (const juce::MouseEvent& e)
{
    applyPaint (noteAt (e.position));
}

void KeyZoneStrip::showFillMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Fill key map");
    m.addItem (1, "All CS-80");
    m.addItem (2, "All JP-8");
    m.addItem (3, "All layered (both)");
    m.addSeparator();
    m.addItem (4, "CS-80 low / JP-8 high at split point");
    m.addItem (5, "JP-8 low / CS-80 high at split point");
    m.addItem (6, "Alternate octaves");
    m.addItem (7, "Alternate notes");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
        [this] (int result)
        {
            const int split = (int) proc.apvts.getRawParameterValue (ID::splitPoint)->load();
            switch (result)
            {
                case 1: proc.fillKeyZones ([] (int)  { return (uint8_t) 0; }); break;
                case 2: proc.fillKeyZones ([] (int)  { return (uint8_t) 1; }); break;
                case 3: proc.fillKeyZones ([] (int)  { return (uint8_t) 2; }); break;
                case 4: proc.fillKeyZones ([split] (int n) { return (uint8_t) (n < split ? 0 : 1); }); break;
                case 5: proc.fillKeyZones ([split] (int n) { return (uint8_t) (n < split ? 1 : 0); }); break;
                case 6: proc.fillKeyZones ([] (int n) { return (uint8_t) ((n / 12) % 2); }); break;
                case 7: proc.fillKeyZones ([] (int n) { return (uint8_t) (n % 2); }); break;
                default: return;
            }
            repaint();
        });
}

// ================================================================ SeqGrid
namespace
{
    juce::String noteName (int n) { return juce::MidiMessage::getMidiNoteName (n, true, true, 3); }

    juce::String chordText (const eighty::SynthEngine::SeqStep& st)
    {
        juce::String s;
        for (uint8_t i = 0; i < st.count; ++i)
        {
            if (i > 0) s << " ";
            s << noteName (st.note[i]);
        }
        return s;
    }
}

SeqGrid::SeqGrid (EightyProcessor& p) : proc (p)
{
    setTooltip ("Step grid. Click a step to move the record/edit cursor there (and drop in any keys "
                "you are holding). Drag up/down to transpose a step, drag left/right to hold it "
                "across several steps, double-click to clear it, right-click for the step and "
                "track menu");
}

int SeqGrid::trackAt (juce::Point<int> pos) const
{
    if (pos.y < rulerH) return -1;
    return pos.y < rulerH + rowH + rowGap / 2 ? 0 : 1;
}

int SeqGrid::stepAt (juce::Point<int> pos) const
{
    const int n = eighty::SynthEngine::SeqTrack::kSteps;
    return juce::jlimit (0, n - 1, pos.x * n / juce::jmax (1, getWidth()));
}

juce::Rectangle<int> SeqGrid::cellBounds (int track, int step) const
{
    const int n = eighty::SynthEngine::SeqTrack::kSteps;
    const int x0 = step * getWidth() / n;
    const int x1 = (step + 1) * getWidth() / n;
    const int y = rulerH + track * (rowH + rowGap);
    return { x0, y, x1 - x0, rowH };
}

void SeqGrid::paint (juce::Graphics& g)
{
    auto& seq = proc.engine.seq;
    const int nSteps = eighty::SynthEngine::SeqTrack::kSteps;
    const bool playing = proc.engine.seqPlay;
    const bool recording = proc.engine.seqRec;

    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.f);

    // ruler: step numbers, bar accents every 4
    g.setFont (ui::mono (8.5f));
    for (int s = 0; s < nSteps; ++s)
    {
        auto c = cellBounds (0, s);
        g.setColour (s % 4 == 0 ? ui::scopeTrace.withAlpha (0.75f) : ui::dim.withAlpha (0.6f));
        g.drawText (juce::String (s + 1), c.getX(), 1, c.getWidth(), rulerH - 2,
                    juce::Justification::centred);
    }

    for (int t = 0; t < eighty::SynthEngine::StepSeq::kTracks; ++t)
    {
        const auto& track = seq.tracks[(size_t) t];
        const int len = juce::jlimit (1, nSteps, track.length);

        // Walk the loop the way playback does to find which steps are
        // swallowed by a held note - those are drawn under a sustain bar
        // and greyed, because their own contents never fire.
        bool covered[eighty::SynthEngine::SeqTrack::kSteps] = {};
        for (int s = 0; s < len;)
        {
            const auto& st = track.steps[(size_t) s];
            const int h = st.count > 0
                        ? juce::jlimit (1, eighty::SynthEngine::SeqStep::kMaxHold, (int) st.hold)
                        : 1;
            for (int i = 1; i < h; ++i)
                covered[(s + i) % len] = true;
            s += h;
        }

        for (int s = 0; s < nSteps; ++s)
        {
            auto cell = cellBounds (t, s).reduced (1);
            const auto& st = track.steps[(size_t) s];
            const bool beyond = s >= len;
            const bool swallowed = ! beyond && covered[s];
            const bool onPlayhead = playing && ! beyond && track.playStep == s;
            const bool onCursor = seq.recTrack == t && seq.recStep == s;

            // body
            const bool lit = st.count > 0 && ! swallowed;
            juce::Colour fill = beyond ? ui::winBg.withAlpha (0.6f)
                              : lit ? ui::scopeTrace.withAlpha (track.mute ? 0.16f : 0.34f)
                                    : ui::scopeLine.withAlpha (swallowed ? 0.28f : 0.5f);
            if (onPlayhead)
                fill = lit ? ui::scopeTrace.withAlpha (track.mute ? 0.3f : 0.85f)
                           : ui::scopeLine.withAlpha (0.9f);
            g.setColour (fill);
            g.fillRoundedRectangle (cell.toFloat(), 2.f);

            // bar-line accent every 4 steps
            if (s % 4 == 0 && ! beyond)
            {
                g.setColour (ui::scopeTrace.withAlpha (0.25f));
                g.fillRect (cell.getX(), cell.getY(), 2, cell.getHeight());
            }

            if (st.count > 0)
            {
                g.setColour (onPlayhead && lit ? ui::scopeBg
                           : beyond || swallowed ? ui::dimmer.withAlpha (0.7f) : ui::scopeTrace);
                g.setFont (ui::mono (st.count > 3 ? 8.f : 9.5f));
                g.drawText (chordText (st), cell.reduced (3, 0),
                            juce::Justification::centred, false);
            }

            if (onCursor)
            {
                g.setColour (recording ? ui::ledOn : ui::scopeTrace);
                g.drawRoundedRectangle (cell.toFloat().reduced (0.5f), 2.f, 1.6f);
            }
        }

        // sustain bars: a held step draws a tail through the steps it covers,
        // wrapping at the loop end if it runs past it
        for (int s = 0; s < len; ++s)
        {
            const auto& st = track.steps[(size_t) s];
            if (st.count == 0 || st.hold <= 1 || covered[s]) continue;
            const int h = juce::jlimit (1, eighty::SynthEngine::SeqStep::kMaxHold, (int) st.hold);

            g.setColour (ui::scopeTrace.withAlpha (track.mute ? 0.25f : 0.6f));
            for (int i = 1; i < h; ++i)
            {
                const int at = (s + i) % len;
                auto c = cellBounds (t, at).reduced (1);
                const bool wrapped = s + i >= len;
                // start at the left edge unless this tail piece follows on
                // directly from the previous cell
                const int x0 = c.getX() - (at == 0 || wrapped ? 0 : 2);
                g.fillRect (x0, c.getCentreY() - 1, c.getRight() - x0, 3);
            }
            // stub out of the originating cell so the tie reads as connected
            auto from = cellBounds (t, s).reduced (1);
            if (s + 1 < len || h > 1)
                g.fillRect (from.getRight() - 2, from.getCentreY() - 1, 4, 3);
        }

        // loop-end marker, so a shortened track reads at a glance
        if (len < nSteps)
        {
            auto last = cellBounds (t, len - 1);
            g.setColour (ui::ledOn.withAlpha (0.9f));
            g.fillRect (last.getRight() - 1, last.getY(), 2, last.getHeight());
        }

    }
}

void SeqGrid::setCursor (int track, int step)
{
    if (auto* p = proc.apvts.getParameter (ID::seqTrack))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) track));
        p->endChangeGesture();
    }
    proc.engine.seq.recTrack = track;
    proc.engine.seq.recStep = step;
    repaint();
}

void SeqGrid::mouseDown (const juce::MouseEvent& e)
{
    const int t = trackAt (e.getPosition());
    if (t < 0) return;
    const int s = stepAt (e.getPosition());

    if (e.mods.isPopupMenu()) { showStepMenu (t, s); return; }

    setCursor (t, s);
    dragTrack = t; dragStep = s;
    dragStartX = e.x; dragStartY = e.y;
    dragAxis = 0; dragApplied = 0;
    if (const auto* cur = proc.engine.stepAt (t, s)) dragBaseHold = juce::jmax (1, (int) cur->hold);

    // drop whatever is being held into the step - the fastest way to build
    // a pattern without arming REC
    auto* st = proc.engine.stepAt (t, s);
    if (st == nullptr) return;
    eighty::SynthEngine::SeqStep fresh;
    fresh.hold = st->hold;          // hold belongs to the step, not the notes
    for (int n = 0; n < 128; ++n)
        if (const int v = proc.heldNoteVel[(size_t) n].load (std::memory_order_relaxed))
            fresh.add (n, (float) v / 127.f);
    if (fresh.count > 0)
    {
        proc.engine.seqSetStep (t, s, fresh);
        say ("SEQ " + juce::String (t == 0 ? "A" : "B") + " STEP " + juce::String (s + 1)
             + " . " + chordText (fresh));
        repaint();
    }
    else
        say ("SEQ CURSOR . TRACK " + juce::String (t == 0 ? "A" : "B")
             + " STEP " + juce::String (s + 1));
}

// Drag locks to one axis on the first real movement: up/down transposes the
// step, left/right stretches it across the steps it should hold for.
void SeqGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragTrack < 0) return;
    auto* st = proc.engine.stepAt (dragTrack, dragStep);
    if (st == nullptr || st->count == 0) return;

    const int dx = e.x - dragStartX, dy = e.y - dragStartY;
    if (dragAxis == 0)
    {
        if (std::abs (dx) < 8 && std::abs (dy) < 6) return;
        dragAxis = std::abs (dx) > std::abs (dy) ? 2 : 1;
    }

    if (dragAxis == 2)
    {
        const int nSteps = eighty::SynthEngine::SeqTrack::kSteps;
        const int cellW = juce::jmax (1, getWidth() / nSteps);
        const int want = juce::jlimit (1, eighty::SynthEngine::SeqStep::kMaxHold,
                                       dragBaseHold + juce::roundToInt ((float) dx / (float) cellW));
        if (want == (int) st->hold) return;
        proc.engine.seqSetHold (dragTrack, dragStep, want);
        say ("SEQ STEP " + juce::String (dragStep + 1) + " HOLDS "
             + juce::String (want) + (want == 1 ? " STEP" : " STEPS"));
        repaint();
        return;
    }

    const int wanted = -dy / 6;                     // 6 px per semitone
    const int delta = wanted - dragApplied;
    if (delta == 0) return;
    st->transpose (delta);
    dragApplied = wanted;
    say ("SEQ STEP " + juce::String (dragStep + 1) + " . " + chordText (*st));
    repaint();
}

void SeqGrid::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int t = trackAt (e.getPosition());
    if (t < 0) return;
    const int s = stepAt (e.getPosition());
    proc.engine.seqClearStep (t, s);
    say ("SEQ STEP " + juce::String (s + 1) + " CLEARED");
    repaint();
}

void SeqGrid::showStepMenu (int track, int step)
{
    const juce::String tn (track == 0 ? "A" : "B");
    const auto* st = proc.engine.stepAt (track, step);
    const int hold = st != nullptr ? juce::jmax (1, (int) st->hold) : 1;

    juce::PopupMenu holds;
    for (int h = 1; h <= 8; ++h)
        holds.addItem (100 + h, h == 1 ? "1 step (normal)" : juce::String (h) + " steps",
                       true, h == hold);
    holds.addSeparator();
    for (int h : { 12, 16 })
        holds.addItem (100 + h, juce::String (h) + " steps", true, h == hold);

    juce::PopupMenu m;
    m.addSectionHeader ("Track " + tn + ", step " + juce::String (step + 1));
    m.addSubMenu ("Hold for", holds, st != nullptr && st->count > 0);
    m.addItem (1, "Clear step");
    m.addItem (2, "Set loop length to " + juce::String (step + 1) + " steps");
    m.addSeparator();
    m.addItem (3, "Clear track " + tn);
    m.addItem (4, "Copy track " + tn + " to " + (track == 0 ? "B" : "A"));
    m.addItem (5, "Transpose track " + tn + " up an octave");
    m.addItem (6, "Transpose track " + tn + " down an octave");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
        [this, track, step] (int result)
        {
            auto& engine = proc.engine;
            if (result > 100)
            {
                const int h = result - 100;
                engine.seqSetHold (track, step, h);
                say ("SEQ STEP " + juce::String (step + 1) + " HOLDS "
                     + juce::String (h) + (h == 1 ? " STEP" : " STEPS"));
                repaint();
                return;
            }
            switch (result)
            {
                case 1: engine.seqClearStep (track, step); break;
                case 2:
                    if (auto* p = proc.apvts.getParameter (track == 0 ? ID::seqALen : ID::seqBLen))
                    {
                        p->beginChangeGesture();
                        p->setValueNotifyingHost (p->convertTo0to1 ((float) (step + 1)));
                        p->endChangeGesture();
                    }
                    break;
                case 3: engine.seqClearTrack (track); break;
                case 4: engine.seqCopyTrack (track, 1 - track); break;
                case 5: engine.seqTransposeTrack (track, 12); break;
                case 6: engine.seqTransposeTrack (track, -12); break;
                default: return;
            }
            repaint();
        });
}

// ========================================================= PluginChooser
PluginChooser::PluginChooser (juce::Array<juce::PluginDescription> items,
                             const juce::String& t,
                             std::function<void (const juce::PluginDescription&)> onPick,
                             std::function<void()> onRescan)
    : all (std::move (items)), title (t), pick (std::move (onPick)), rescan (std::move (onRescan))
{
    search.setTextToShowWhenEmpty ("type to search...", ui::dim);
    search.setFont (ui::sans (13.f));
    search.setColour (juce::TextEditor::backgroundColourId, ui::groove);
    search.setColour (juce::TextEditor::textColourId, ui::ink);
    search.setColour (juce::TextEditor::outlineColourId, ui::track);
    search.setColour (juce::TextEditor::focusedOutlineColourId, ui::scopeTrace);
    search.setColour (juce::TextEditor::highlightColourId, ui::scopeTrace.withAlpha (0.4f));
    search.setColour (juce::CaretComponent::caretColourId, ui::ink);
    search.addListener (this);
    search.addKeyListener (this);
    addAndMakeVisible (search);

    list.setRowHeight (22);
    list.setColour (juce::ListBox::backgroundColourId, ui::cardBg);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    rescanBtn.setWantsKeyboardFocus (false);
    rescanBtn.onClick = [this] { if (rescan) rescan(); };
    addAndMakeVisible (rescanBtn);

    refilter();
    setSize (420, 340);
}

void PluginChooser::visibilityChanged()
{
    // Deferred: the call-out takes keyboard focus itself as it enters its
    // modal state, which happens after we become visible.
    if (! isShowing()) return;
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<PluginChooser> (this)]
    {
        if (safe != nullptr && safe->isShowing())
            safe->search.grabKeyboardFocus();
    });
}

void PluginChooser::paint (juce::Graphics& g)
{
    g.fillAll (ui::bandBg);
    g.setColour (ui::textHi);
    auto f = ui::sans (11.f, true);
    f.setExtraKerningFactor (0.05f);
    g.setFont (f);
    g.drawText (title, 10, 8, getWidth() - 20, 12, juce::Justification::centredLeft);
    g.setColour (ui::stOsc);
    g.fillRect (10, 22, 26, 3);

    if (filtered.isEmpty())
    {
        g.setColour (ui::dim);
        g.setFont (ui::sans (12.f));
        g.drawText (all.isEmpty() ? "Nothing scanned yet - try Rescan below"
                                  : "No plugin matches that",
                    getLocalBounds().reduced (16), juce::Justification::centred);
    }
}

void PluginChooser::resized()
{
    auto b = getLocalBounds().reduced (10);
    b.removeFromTop (22);
    search.setBounds (b.removeFromTop (26));
    b.removeFromTop (6);
    rescanBtn.setBounds (b.removeFromBottom (22));
    b.removeFromBottom (6);
    list.setBounds (b);
}

void PluginChooser::refilter()
{
    const auto query = search.getText().trim();
    filtered.clearQuick();
    for (int i = 0; i < all.size(); ++i)
    {
        const auto& d = all.getReference (i);
        if (query.isEmpty()
            || d.name.containsIgnoreCase (query)
            || d.manufacturerName.containsIgnoreCase (query))
            filtered.add (i);
    }
    list.updateContent();
    list.selectRow (filtered.isEmpty() ? -1 : 0);
    repaint();
}

void PluginChooser::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= filtered.size()) return;
    const auto& d = all.getReference (filtered[row]);

    if (selected)
    {
        g.setColour (ui::ctrlLine);
        g.fillRect (0, 0, w, h);
    }
    g.setColour (selected ? ui::textHi : ui::ink);
    g.setFont (ui::sans (12.5f));
    g.drawText (d.name, 8, 0, w - 110, h, juce::Justification::centredLeft, true);
    g.setColour (selected ? ui::textHi.withAlpha (0.7f) : ui::dim);
    g.setFont (ui::sans (10.f));
    g.drawText (d.manufacturerName, w - 100, 0, 92, h, juce::Justification::centredRight, true);
}

void PluginChooser::listBoxItemDoubleClicked (int row, const juce::MouseEvent&) { choose (row); }
void PluginChooser::returnKeyPressed (int row) { choose (row); }
void PluginChooser::textEditorTextChanged (juce::TextEditor&) { refilter(); }
void PluginChooser::textEditorReturnKeyPressed (juce::TextEditor&) { choose (list.getSelectedRow()); }
void PluginChooser::textEditorEscapeKeyPressed (juce::TextEditor&) { dismiss(); }

// Up/down steer the list while the caret stays in the search box
bool PluginChooser::keyPressed (const juce::KeyPress& kp, juce::Component*)
{
    const int code = kp.getKeyCode();
    if (code != juce::KeyPress::upKey && code != juce::KeyPress::downKey) return false;
    if (filtered.isEmpty()) return true;

    const int row = juce::jlimit (0, filtered.size() - 1,
                                  list.getSelectedRow() + (code == juce::KeyPress::downKey ? 1 : -1));
    list.selectRow (row);
    return true;
}

void PluginChooser::choose (int row)
{
    if (row < 0 || row >= filtered.size()) return;
    auto desc = all.getReference (filtered[row]);
    auto fn = pick;
    dismiss();                       // the call-out owns us: copy first, then die
    if (fn) fn (desc);
}

void PluginChooser::dismiss()
{
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
        box->dismiss();
}

// ============================================================ InsertPanel
namespace
{
class HostedPluginWindow : public juce::DocumentWindow
{
public:
    HostedPluginWindow (juce::AudioPluginInstance& instance,
                        std::function<void (HostedPluginWindow*)> onClose)
        : DocumentWindow (instance.getName(), ui::winBg, DocumentWindow::closeButton),
          inst (&instance), closeFn (std::move (onClose))
    {
        setUsingNativeTitleBar (true);
        if (auto* ed = instance.createEditorIfNeeded())
            setContentOwned (ed, true);
        else
            setContentOwned (new juce::GenericAudioProcessorEditor (instance), true);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
        setAlwaysOnTop (true);
    }
    void closeButtonPressed() override { closeFn (this); }
    juce::AudioPluginInstance* inst;
private:
    std::function<void (HostedPluginWindow*)> closeFn;
};
} // namespace

InsertPanel::InsertPanel (EightyProcessor& p, std::function<void (const juce::String&)> touched)
    : proc (p)
{
    addBtn.setWantsKeyboardFocus (false);
    addBtn.setTooltip ("Load a VST3 effect at the end of the chain (post-FX, pre volume)");
    addBtn.onClick = [this] { showChooser (false); };
    addAndMakeVisible (addBtn);

    synthBtn.setWantsKeyboardFocus (false);
    synthBtn.setTooltip ("Load any VST3 instrument as a third layer. It follows the arp, hold and pitch wheel");
    synthBtn.onClick = [this]
    {
        if (auto* s = proc.getSynthLayer())
            openWindowFor (s);
        else
            showChooser (true);
    };
    addAndMakeVisible (synthBtn);

    synthClearBtn.setWantsKeyboardFocus (false);
    synthClearBtn.setTooltip ("Remove the synth layer");
    synthClearBtn.onClick = [this]
    {
        closeWindowFor (proc.getSynthLayer());
        proc.clearSynthLayer();
    };
    addChildComponent (synthClearBtn);

    // The synth layer's level, mute and solo live in the mixer card next
    // door now, alongside the two engines they are balanced against.
    juce::ignoreUnused (touched);

    startTimerHz (10);
    refresh();
}

InsertPanel::~InsertPanel()
{
    windows.clear();
}

void InsertPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (ui::cardBg);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (ui::line);
    g.drawRoundedRectangle (r, 4.f, 1.f);

    g.setColour (ui::dim);
    g.setFont (ui::sans (8.5f, true));
    g.drawText ("SYNTH LAYER", 10, 4, 120, 10, juce::Justification::centredLeft);
    g.drawText ("VST3 INSERTS", 10, 40, 120, 10, juce::Justification::centredLeft);
}

void InsertPanel::timerCallback()
{
    const int v = proc.chainVersion.load();
    if (v != lastVersion)
    {
        lastVersion = v;
        refresh();
    }
}

void InsertPanel::refresh()
{
    for (int w = windows.size(); --w >= 0;)
    {
        auto* hw = static_cast<HostedPluginWindow*> (windows[w]);
        bool alive = hw->inst == proc.getSynthLayer();
        for (int i = 0; i < proc.getNumInserts() && ! alive; ++i)
            alive = proc.getInsert (i) == hw->inst;
        if (! alive)
            windows.remove (w);
    }

    const auto synthName = proc.getSynthLayerName();
    synthBtn.setButtonText (synthName.isEmpty() ? "+ SET SYNTH" : synthName);
    synthClearBtn.setVisible (synthName.isNotEmpty());

    openBtns.clear();
    removeBtns.clear();
    for (int i = 0; i < proc.getNumInserts(); ++i)
    {
        auto* open = openBtns.add (new juce::TextButton (
            juce::String (i + 1) + " " + proc.getInsertName (i)));
        open->setWantsKeyboardFocus (false);
        open->setTooltip ("Open this plugin's editor");
        open->onClick = [this, i] { openWindowFor (proc.getInsert (i)); };
        addAndMakeVisible (open);

        auto* rm = removeBtns.add (new juce::TextButton ("x"));
        rm->setWantsKeyboardFocus (false);
        rm->setTooltip ("Remove from the chain");
        rm->onClick = [this, i]
        {
            closeWindowFor (proc.getInsert (i));
            proc.removeInsert (i);
        };
        addAndMakeVisible (rm);
    }
    addBtn.setEnabled (proc.getNumInserts() < EightyProcessor::kMaxInserts);
    resized();
    repaint();
}

void InsertPanel::resized()
{
    auto b = getLocalBounds().reduced (8, 3);
    b.removeFromTop (12);
    auto synthRow = b.removeFromTop (20);
    synthClearBtn.setBounds (synthRow.removeFromRight (20).reduced (1));
    synthBtn.setBounds (synthRow.reduced (1));

    b.removeFromTop (13);
    const int rowH = 18;
    for (int i = 0; i < openBtns.size(); ++i)
    {
        auto row = b.removeFromTop (rowH);
        removeBtns[i]->setBounds (row.removeFromRight (rowH).reduced (1));
        openBtns[i]->setBounds (row.reduced (1));
    }
    addBtn.setBounds (b.removeFromTop (rowH).reduced (1));
}

void InsertPanel::showChooser (bool instruments)
{
    juce::Array<juce::PluginDescription> matching;
    for (auto& t : proc.knownPlugins.getTypes())
        if (t.isInstrument == instruments && t.name != "Eighty")
            matching.add (t);

    auto load = [this, instruments] (const juce::PluginDescription& desc)
    {
        juce::String error;
        const bool ok = instruments ? proc.setSynthLayer (desc, error)
                                    : proc.addInsert (desc, error);
        if (! ok)
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon, "Couldn't load plugin", error);
    };

    // Rescan is blocking, so close the call-out first and reopen it after
    auto again = [safe = juce::Component::SafePointer<InsertPanel> (this), instruments]
    {
        if (safe == nullptr) return;
        juce::MouseCursor::showWaitCursor();
        safe->proc.rescanPlugins();
        juce::MouseCursor::hideWaitCursor();
        if (safe != nullptr) safe->showChooser (instruments);
    };

    auto content = std::make_unique<PluginChooser> (
        std::move (matching),
        instruments ? "SYNTH LAYER" : "VST3 EFFECT",
        std::move (load),
        [again] { juce::MessageManager::callAsync (again); });

    auto& button = instruments ? synthBtn : addBtn;
    juce::CallOutBox::launchAsynchronously (std::move (content), button.getScreenBounds(), nullptr);
}

void InsertPanel::openWindowFor (juce::AudioPluginInstance* inst)
{
    if (inst == nullptr) return;
    for (auto* w : windows)
        if (static_cast<HostedPluginWindow*> (w)->inst == inst)
        {
            w->toFront (true);
            return;
        }
    windows.add (new HostedPluginWindow (*inst,
        [this] (HostedPluginWindow* w) { windows.removeObject (w); }));
}

void InsertPanel::closeWindowFor (juce::AudioPluginInstance* inst)
{
    for (int w = windows.size(); --w >= 0;)
        if (static_cast<HostedPluginWindow*> (windows[w])->inst == inst)
            windows.remove (w);
}

// ================================================================= Editor
EightyEditor::EightyEditor (EightyProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      scopeTap (p.scopeFifo),
      scope (scopeTap),
      lissajous (scopeTap),
      insertPanel (p, [this] (const juce::String& id) { paramTouched (id); }),
      seqGrid (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    // ---- header
    titleLabel.setText ("EIGHTY", juce::dontSendNotification);
    {
        auto f = ui::sans (20.f);
        f.setExtraKerningFactor (0.3f);
        titleLabel.setFont (f);
    }
    titleLabel.setColour (juce::Label::textColourId, ui::ink);
    addAndMakeVisible (titleLabel);

    subLabel.setText ("CS-80 x JUPITER-8", juce::dontSendNotification);
    {
        auto f = ui::sans (8.f, true);
        f.setExtraKerningFactor (0.24f);
        subLabel.setFont (f);
    }
    subLabel.setColour (juce::Label::textColourId, ui::dim);
    addAndMakeVisible (subLabel);

    // LCD readout (tab band, dark background painted by the editor)
    {
        auto f = ui::mono (11.f);
        f.setExtraKerningFactor (0.06f);
        statusLabel.setFont (f);
    }
    statusLabel.setColour (juce::Label::textColourId, ui::scopeTrace);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    hintLabel.setText ("PLAY: A W S E D F T G Y H U J K O L P ;    Z/X OCTAVE    C/V VELOCITY    "
                       "B HOLD    N ARP    M SEQ PLAY    R SEQ REC    LEFT/RIGHT SELECT    "
                       "UP/DOWN ADJUST (HOLD TO SWEEP)    SHIFT+UP/DOWN BEND    RIGHT-CLICK: MIDI LEARN",
                       juce::dontSendNotification);
    hintLabel.setFont (ui::mono (9.5f));
    hintLabel.setColour (juce::Label::textColourId, ui::dim);
    hintLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (hintLabel);

    // engine mode chips
    if (auto* param = proc.apvts.getParameter (ID::engineMode))
    {
        engineChips = std::make_unique<ChipStack> (*param,
            juce::StringArray { "CS-80", "JP-8", "SPLIT", "LAYER", "KEYS" },
            juce::String(), true, ui::stOsc);
        engineChips->setTooltip (tipFor (ID::engineMode));
        engineChips->onUserChange = [this] { paramTouched (ID::engineMode); };
        engineChips->onRightClick = [this] (juce::Point<int> pos)
        { showLearnMenu (ID::engineMode, pos); };
        controls.push_back ({ engineChips.get(), ID::engineMode });
        engineChips->addMouseListener (this, true);
        addAndMakeVisible (*engineChips);
    }

    panelChips.onSelect = [this] (bool jp) { setEngineView (jp); };
    addAndMakeVisible (panelChips);

    splitKnob = makeHeaderKnob (ID::splitPoint, "SPLIT");
    splitKnob->slider.textFromValueFunction = [] (double v)
    { return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 3); };
    if (auto* param = proc.apvts.getParameter (ID::splitCsLow))
    {
        csLowLed = std::make_unique<LedToggle> (*param, "CS LOW");
        csLowLed->setTooltip (tipFor (ID::splitCsLow));
        csLowLed->onUserChange = [this] { paramTouched (ID::splitCsLow); };
        csLowLed->onRightClick = [this] (juce::Point<int> pos)
        { showLearnMenu (ID::splitCsLow, pos); };
        controls.push_back ({ csLowLed.get(), ID::splitCsLow });
        csLowLed->addMouseListener (this, true);
        addAndMakeVisible (*csLowLed);
    }
    balKnob   = makeHeaderKnob (ID::engineBalance, "CS/JP");
    volKnob   = makeHeaderKnob (ID::masterVol, "VOLUME");
    tuneKnob  = makeHeaderKnob (ID::masterTune, "TUNE");
    limitKnob = makeHeaderKnob (ID::limitDrive, "LIMIT");
    widthKnob = makeHeaderKnob (ID::stereoWidth, "WIDTH");

    // ---- CS-80 engine row
    makeLed   (secOsc1, ID::osc1On, "ON");
    makeChips (secOsc1, ID::osc1Foot, { "32'", "16'", "8'", "4'" }, "RANGE");
    makeFader (secOsc1, ID::osc1Saw, "SAW");
    makeFader (secOsc1, ID::osc1Pulse, "PULSE");
    makeFader (secOsc1, ID::osc1Level, "LEVEL");
    makeKnob  (secOsc1, ID::osc1PW, "PW");
    makeKnob  (secOsc1, ID::osc1PWM, "PWM");
    makeKnob  (secOsc1, ID::osc1Fine, "FINE");
    addAndMakeVisible (secOsc1);

    makeLed   (secOsc2, ID::osc2On, "ON");
    makeChips (secOsc2, ID::osc2Foot, { "32'", "16'", "8'", "4'" }, "RANGE");
    makeFader (secOsc2, ID::osc2Saw, "SAW");
    makeFader (secOsc2, ID::osc2Pulse, "PULSE");
    makeFader (secOsc2, ID::osc2Level, "LEVEL");
    makeKnob  (secOsc2, ID::osc2Semi, "SEMI");
    makeKnob  (secOsc2, ID::osc2Fine, "FINE");
    makeKnob  (secOsc2, ID::osc2PW, "PW");
    makeKnob  (secOsc2, ID::osc2PWM, "PWM");
    addAndMakeVisible (secOsc2);

    makeFader (secMix, ID::noiseLevel, "NOISE");
    makeFader (secMix, ID::csSubLevel, "SUB");
    // The sine leaves the mixer *after* the filters, as on the original
    makeFader (secMix, ID::csSineLevel, "SINE");
    makeChips (secMix, ID::csSubOct, { "-1", "-2" }, "OCT", 34);
    makeChips (secMix, ID::csSubWave, { "SQR", "TRI" }, "WAVE", 34);
    makeKnob  (secMix, ID::pwmRate, "PWM RT");
    addAndMakeVisible (secMix);

    makeFader (secFilter, ID::hpfCutoff, "HPF");
    makeFader (secFilter, ID::hpfRes, "H.RES");
    makeFader (secFilter, ID::lpfCutoff, "LPF");
    makeFader (secFilter, ID::resonance, "RES");
    makeFader (secFilter, ID::filterEnvAmt, "ENV");
    makeKnob  (secFilter, ID::keyTrack, "KEY TRK");
    makeKnob  (secFilter, ID::filterDrive, "DRIVE");
    // Both corners and both resonances: the CS-80 filter puts resonance on
    // the high pass as well, and the display is the only place that shows it.
    makeDisplay (secFilter, MiniDisplay::filterKind,
                 { ID::hpfCutoff, ID::lpfCutoff, ID::resonance, ID::hpfRes }, 80);
    addAndMakeVisible (secFilter);

    // The CS-80 filter EG is not an ADSR: IL is where it starts from and AL
    // is where the attack peaks. Raising IL shortens the sweep and dulls the
    // tone; lowering it enriches it.
    makeFader (secFEnv, ID::fEnvIL, "IL");
    makeFader (secFEnv, ID::fEnvA, "A");
    makeFader (secFEnv, ID::fEnvD, "D");
    makeFader (secFEnv, ID::fEnvS, "S");
    makeFader (secFEnv, ID::fEnvR, "R");
    makeFader (secFEnv, ID::fEnvAL, "AL");
    makeKnob  (secFEnv, ID::velToFilter, "VEL");
    // IL/AL feed the display too, so the trace starts and peaks where the
    // CS-80's non-ADSR filter EG actually does
    makeDisplay (secFEnv, MiniDisplay::adsrKind,
                 { ID::fEnvA, ID::fEnvD, ID::fEnvS, ID::fEnvR, ID::fEnvIL, ID::fEnvAL }, 88);
    addAndMakeVisible (secFEnv);

    makeFader (secAEnv, ID::aEnvA, "A");
    makeFader (secAEnv, ID::aEnvD, "D");
    makeFader (secAEnv, ID::aEnvS, "S");
    makeFader (secAEnv, ID::aEnvR, "R");
    makeKnob  (secAEnv, ID::velToAmp, "VEL");
    makeDisplay (secAEnv, MiniDisplay::adsrKind, { ID::aEnvA, ID::aEnvD, ID::aEnvS, ID::aEnvR }, 88);
    addAndMakeVisible (secAEnv);

    // ---- Jupiter-8 engine row
    makeChips (secVco1, ID::jpVco1Wave, { "TRI", "SAW", "PLS", "SQR" }, "WAVE");
    makeChips (secVco1, ID::jpVco1Range, { "16'", "8'", "4'", "2'" }, "RANGE");
    makeKnob  (secVco1, ID::jpPW, "PW");
    makeKnob  (secVco1, ID::jpPWM, "PWM");
    addAndMakeVisible (secVco1);

    makeLed   (secVco2, ID::jpSync, "SYNC");
    // LOW drops VCO-2 out of keyboard tracking and runs it at LF, which is
    // how most Jupiter-8 sync and cross-mod patches are actually built
    makeLed   (secVco2, ID::jpVco2Low, "LOW");
    makeChips (secVco2, ID::jpVco2Wave, { "TRI", "SAW", "PLS", "NSE" }, "WAVE");
    makeChips (secVco2, ID::jpVco2Range, { "16'", "8'", "4'", "2'" }, "RANGE");
    makeKnob  (secVco2, ID::jpVco2Semi, "SEMI");
    makeKnob  (secVco2, ID::jpVco2Fine, "FINE");
    makeKnob  (secVco2, ID::jpXmod, "X-MOD");
    addAndMakeVisible (secVco2);

    makeFader (secJpMix, ID::jpMix, "MIX");
    makeFader (secJpMix, ID::jpSubLevel, "SUB");
    makeChips (secJpMix, ID::jpSubOct, { "-1", "-2" }, "OCT", 34);
    makeChips (secJpMix, ID::jpSubWave, { "SQR", "TRI" }, "WAVE", 34);
    makeKnob  (secJpMix, ID::pwmRate, "PWM RT");
    addAndMakeVisible (secJpMix);

    // ENV-1 polarity lives here rather than on F.ENV: that section carries a
    // mini display in the same top-right slot the LED toggles use.
    makeLed   (secJpFilter, ID::jpEnvInv, "ENV INV");
    makeLed   (secJpFilter, ID::jpSlope24, "24dB");
    makeFader (secJpFilter, ID::jpHpf, "HPF");
    makeFader (secJpFilter, ID::jpLpf, "LPF");
    makeFader (secJpFilter, ID::jpRes, "RES");
    makeFader (secJpFilter, ID::jpEnvAmt, "ENV");
    makeKnob  (secJpFilter, ID::jpKeyTrk, "KEY TRK");
    makeKnob  (secJpFilter, ID::jpDrive, "DRIVE");
    addAndMakeVisible (secJpFilter);

    makeFader (secJpFEnv, ID::jpFEnvA, "A");
    makeFader (secJpFEnv, ID::jpFEnvD, "D");
    makeFader (secJpFEnv, ID::jpFEnvS, "S");
    makeFader (secJpFEnv, ID::jpFEnvR, "R");
    makeKnob  (secJpFEnv, ID::velToFilter, "VEL");
    makeDisplay (secJpFEnv, MiniDisplay::adsrKind,
                 { ID::jpFEnvA, ID::jpFEnvD, ID::jpFEnvS, ID::jpFEnvR }, 88);
    addAndMakeVisible (secJpFEnv);

    makeFader (secJpAEnv, ID::jpAEnvA, "A");
    makeFader (secJpAEnv, ID::jpAEnvD, "D");
    makeFader (secJpAEnv, ID::jpAEnvS, "S");
    makeFader (secJpAEnv, ID::jpAEnvR, "R");
    makeKnob  (secJpAEnv, ID::velToAmp, "VEL");
    makeDisplay (secJpAEnv, MiniDisplay::adsrKind,
                 { ID::jpAEnvA, ID::jpAEnvD, ID::jpAEnvS, ID::jpAEnvR }, 88);
    addAndMakeVisible (secJpAEnv);

    // ---- shared row
    makeChips (secLfo, ID::lfoWave, { "SIN", "TRI", "SQR", "SAW", "S&H" }, "WAVE", 44);
    makeFader (secLfo, ID::lfoRate, "RATE", 26);
    makeFader (secLfo, ID::lfoDelay, "DELAY", 26);
    makeFader (secLfo, ID::lfoToPitch, "PITCH", 26);
    makeFader (secLfo, ID::lfoToFilter, "FILT", 26);
    makeFader (secLfo, ID::lfoToAmp, "AMP", 26);
    makeDisplay (secLfo, MiniDisplay::lfoKind, { ID::lfoWave }, 66);
    addAndMakeVisible (secLfo);

    makeFader (secTouch, ID::touchRise, "RISE", 26);
    makeFader (secTouch, ID::touchToVib, "VIB", 26);
    makeFader (secTouch, ID::touchToBright, "BRIGHT", 26);
    makeFader (secTouch, ID::touchToLevel, "LEVEL", 26);
    addAndMakeVisible (secTouch);

    // Each engine carries its own voice mode *and* its own portamento, so a
    // JP-8 lead can slide over a CS-80 pad that doesn't.
    makeChips (secVoiceCS, ID::csVoiceMode, { "POLY", "MONO", "LEG", "UNI", "STK" }, "MODE", 44);
    makeKnob  (secVoiceCS, ID::csPolyVoices, "VOICES", 33);
    makeKnob  (secVoiceCS, ID::csUnisonCount, "UNI CNT", 33);
    makeKnob  (secVoiceCS, ID::csUnisonDetune, "DETUNE", 33);
    makeChips (secVoiceCS, ID::csGlideMode, { "OFF", "LEG", "ALW" }, "GLIDE", 38);
    makeKnob  (secVoiceCS, ID::csGlideTime, "TIME", 33);
    addAndMakeVisible (secVoiceCS);

    makeChips (secVoiceJP, ID::jpVoiceMode, { "POLY", "MONO", "LEG", "UNI", "STK" }, "MODE", 44);
    makeKnob  (secVoiceJP, ID::jpPolyVoices, "VOICES", 33);
    makeKnob  (secVoiceJP, ID::jpUnisonCount, "UNI CNT", 33);
    makeKnob  (secVoiceJP, ID::jpUnisonDetune, "DETUNE", 33);
    makeChips (secVoiceJP, ID::jpGlideMode, { "OFF", "LEG", "ALW" }, "GLIDE", 38);
    makeKnob  (secVoiceJP, ID::jpGlideTime, "TIME", 33);
    addAndMakeVisible (secVoiceJP);

    makeKnob  (secGlide, ID::stereoSpread, "SPREAD", 33);
    makeKnob  (secGlide, ID::drift, "DRIFT", 33);
    makeKnob  (secGlide, ID::bendRange, "BEND", 33);
    addAndMakeVisible (secGlide);

    makeLed   (secArp, ID::csArpOn, "CS");
    makeLed   (secArp, ID::jpArpOn, "JP");
    makeLed   (secArp, ID::hold, "HOLD");
    makeLed   (secArp, ID::arpSync, "SYNC");
    makeChips (secArp, ID::arpMode, { "UP", "DN", "UD", "RND", "PLY" }, "MODE", 44);
    makeChips (secArp, ID::arpDiv, { "1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32" }, "DIV", 44);
    // The rate itself is now TEMPO, in the sequencer row: the arp and the
    // sequencer share one step clock, so there is one place to set it.
    makeKnob  (secArp, ID::arpOctaves, "OCT", 33);
    makeChips (secArp, ID::arpMult, { "x1", "x2", "x3", "x4", "x6", "x8" }, "RATE", 40);
    makeKnob  (secArp, ID::arpGate, "GATE", 33);
    addAndMakeVisible (secArp);

    // ---- performance row
    // The CS-80's second channel is a complete synth of its own. Rather than
    // duplicate the whole panel, it runs the same settings offset: its own
    // filter, its own pair of envelope generators, and these four controls
    // against channel I. All-zero is the old single-filter behaviour.
    makeKnob (secCh2, ID::csCh2Cut,  "CUTOFF", 33);
    makeKnob (secCh2, ID::csCh2Res,  "RES", 33);
    makeKnob (secCh2, ID::csCh2Env,  "ENV", 33);
    makeKnob (secCh2, ID::csCh2Time, "TIME", 33);
    addAndMakeVisible (secCh2);

    makeLed  (secRing, ID::ringOn, "ON");
    makeKnob (secRing, ID::ringDepth,  "DEPTH", 33);
    makeKnob (secRing, ID::ringRate,   "RATE", 33);
    makeKnob (secRing, ID::ringEnvAmt, "ENV", 33);
    makeKnob (secRing, ID::ringAtk,    "ATK", 33);
    makeKnob (secRing, ID::ringDec,    "DEC", 33);
    addAndMakeVisible (secRing);

    // BRILLIANCE and RESONANCE are the CS-80's two most-used performance
    // sliders: one offset over every filter cutoff in the instrument, one
    // over every resonance, on top of whatever the patch says.
    makeKnob (secPerform, ID::brilliance, "BRILL", 33);
    makeKnob (secPerform, ID::resOffset,  "RES", 33);
    makeKnob (secPerform, ID::velBend,    "V.BEND", 33);
    addAndMakeVisible (secPerform);

    makeChips (secQuality, ID::oversample, { "OFF", "2x", "4x", "8x" }, "OVERSAMPLE", 66);
    addAndMakeVisible (secQuality);

    makeLed  (secFx, ID::chorusOn, "CHORUS");
    makeLed  (secFx, ID::delayOn, "DELAY");
    makeLed  (secFx, ID::tremOn, "TREM");
    makeLed  (secFx, ID::delaySync, "D.SYNC");
    makeChips (secFx, ID::chorusMode, { "I", "II", "ENS" }, "CHORUS", 44);
    makeKnob (secFx, ID::chorusRate, "C.RATE", 33);
    makeKnob (secFx, ID::chorusDepth, "C.DEP", 33);
    makeKnob (secFx, ID::chorusMix, "C.MIX", 33);
    makeKnob (secFx, ID::delayTime, "D.TIME", 33);
    makeKnob (secFx, ID::delayDiv, "D.DIV", 33);
    makeKnob (secFx, ID::delayFB, "D.FB", 33);
    makeKnob (secFx, ID::delayMix, "D.MIX", 33);
    makeKnob (secFx, ID::tremRate, "T.RATE", 33);
    makeKnob (secFx, ID::tremDepth, "T.DEP", 33);
    addAndMakeVisible (secFx);

    buildSeqRow();
    buildMixer();
    buildPresetBar();
    buildRecorder();

    addAndMakeVisible (scope);
    addAndMakeVisible (lissajous);

    wheel.onChange = [this] (float v, bool active)
    {
        wheelActive = active;
        proc.uiBend.store (v);
        proc.uiBendActive.store (active || bendDir != 0);
        if (! active && bendDir == 0)
        {
            proc.uiBend.store (0.f);
            proc.engine.setBendNorm (0.f);
        }
    };
    wheel.setTooltip ("Pitch bend: drag, springs back on release. Also Shift+Up/Down arrows");
    addAndMakeVisible (wheel);

    keyboard.setWantsKeyboardFocus (false);
    keyboard.setAvailableRange (36, 96);        // 36 white keys, no scrolling
    keyboard.setLowestVisibleKey (36);
    keyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (insertPanel);

    zoneStrip = std::make_unique<KeyZoneStrip> (proc, keyboard);
    addChildComponent (*zoneStrip);

    addChildComponent (halo);
    halo.setAlwaysOnTop (true);

    setEngineView (false);
    updateModeVisibility ((int) proc.apvts.getRawParameterValue (ID::engineMode)->load());
    updateSeqReadout();            // so the strip is filled before the first tick
    setupTrackpad();
    setWantsKeyboardFocus (true);
    addKeyListener (this);
    startTimerHz (30);
    setSize (ui::windowW, ui::windowH);
}

EightyEditor::~EightyEditor()
{
    // Before anything else: the pad captures events app-wide and can be
    // holding the pointer frozen, so it has to be let go first.
    setGestureMode (eighty::GestureEngine::Mode::off);
    trackpad.detach();
    removeKeyListener (this);
    setLookAndFeel (nullptr);
}

// ------------------------------------------------------ trackpad gestures
float EightyEditor::gestureParam (const char* id) const
{
    return proc.apvts.getRawParameterValue (id)->load();
}

void EightyEditor::setupTrackpad()
{
    settingsBtn.setWantsKeyboardFocus (false);
    settingsBtn.setTooltip ("Trackpad performance: strum mode, the ribbon, and how "
                            "they behave");
    settingsBtn.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible (settingsBtn);

    touchMonitor = std::make_unique<TouchMonitor> (proc, gesture);
    addChildComponent (*touchMonitor);

    // What the strum plays. The engine hands back its *latched* set, which
    // is why holding a chord with B (or leaving one under the arpeggiator)
    // is all it takes to have something to strum.
    gesture.cb.chord = [this] (int* dest, int maxDest, int octaves)
    { return proc.engine.strumChord (dest, maxDest, octaves); };

    auto push = [this] (const GestureMsg& m) { proc.gestures.push (m); };

    gesture.cb.note = [push] (int lane, int note, float vel, int model)
    {
        GestureMsg m;
        m.type = GestureMsg::kNote;
        m.lane = (int8_t) lane;
        m.model = (int8_t) model;
        m.note = note;
        m.value = vel;
        push (m);
    };

    gesture.cb.patternStep = [push] (int lane, int dir)
    {
        GestureMsg m;
        m.type = GestureMsg::kPatternStep;
        m.lane = (int8_t) lane;
        m.dir = dir;
        push (m);
    };

    gesture.cb.laneOff = [push] (int lane, bool damp)
    {
        GestureMsg m;
        m.type = GestureMsg::kLaneOff;
        m.lane = (int8_t) lane;
        m.flag = damp;
        push (m);
    };

    gesture.cb.ribbon = [push] (int model, float semis)
    {
        GestureMsg m;
        m.type = GestureMsg::kRibbon;
        m.model = (int8_t) model;
        m.value = semis;
        push (m);
    };

    gesture.cb.cross = [push] (int dest, float value)
    {
        if (dest != eighty::GestureEngine::crossBright) return;
        GestureMsg m;
        m.type = GestureMsg::kBright;
        m.value = value;
        push (m);
    };

    gesture.cb.pressure = [push] (float value)
    {
        GestureMsg m;
        m.type = GestureMsg::kPressure;
        m.value = value;
        push (m);
    };

    trackpad.onFrame = [this] (const eighty::TouchFrame& f) { gesture.frame (f); };
}

// ========================================================== TouchMonitor
TouchMonitor::TouchMonitor (EightyProcessor& p, const eighty::GestureEngine& g)
    : proc (p), gesture (g)
{
    // Purely a readout - it must never take a click away from the panel it
    // is sitting over.
    setInterceptsMouseClicks (false, false);
    setAlwaysOnTop (true);
}

void TouchMonitor::setActive (bool on)
{
    if (on == isVisible()) return;
    setVisible (on);
    // Fingers move far faster than the editor's 30 Hz housekeeping tick, so
    // this runs its own clock - but only while it is on screen.
    if (on) { toFront (false); startTimerHz (60); }
    else      stopTimer();
}

juce::Colour TouchMonitor::modelColour (int model)
{
    return model == eighty::modelJP8 ? ui::jpAccent
         : model == eighty::modelCS80 ? ui::stOsc
                              : ui::stVoice;      // follows the engine mode
}

// along = position on the strum axis, cross = the other one. Both 0..1, and
// both measured the way the trackpad reports them - y runs from the near
// edge upwards, so it is flipped into screen space here and nowhere else.
juce::Point<float> TouchMonitor::padPoint (juce::Rectangle<float> pad,
                                           float along, float cross) const
{
    const bool alongX = gesture.cfg.axis == 0;
    const float x = alongX ? along : cross;
    const float y = alongX ? cross : along;
    return { pad.getX() + x * pad.getWidth(),
             pad.getBottom() - y * pad.getHeight() };
}

void TouchMonitor::paintPad (juce::Graphics& g, juce::Rectangle<float> pad,
                             const int* notes, int numNotes, int slots)
{
    const bool ribbon = gesture.mode() == eighty::GestureEngine::Mode::ribbon;
    const bool alongX = gesture.cfg.axis == 0;

    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (pad, 4.f);
    g.setColour (ui::track);
    g.drawRoundedRectangle (pad.reduced (0.5f), 4.f, 1.f);

    const float span = alongX ? pad.getWidth() : pad.getHeight();

    if (ribbon)
    {
        // No strings on a ribbon, so the pad has to carry its own scale.
        const auto range = juce::String ((int) gesture.cfg.ribbonRange);
        g.setColour (ui::dimmer);
        g.setFont (ui::mono (8.f));
        auto edge = pad.reduced (5.f, 4.f).toNearestInt();
        if (alongX)
        {
            g.drawText ("-" + range, edge, juce::Justification::bottomLeft, false);
            g.drawText ("+" + range, edge, juce::Justification::bottomRight, false);
        }
        else
        {
            g.drawText ("+" + range, edge, juce::Justification::topRight, false);
            g.drawText ("-" + range, edge, juce::Justification::bottomRight, false);
        }

        // What matters is the reference the bend is measured from and how far
        // the finger has travelled off it.
        for (int i = 0; i < gesture.laneCount(); ++i)
        {
            const auto L = gesture.laneState (i);
            if (! L.active) continue;
            const auto col = modelColour (L.model);
            const auto ref = padPoint (pad, L.origin, 0.5f);
            const auto now = padPoint (pad, L.pos, 0.5f);

            g.setColour (ui::scopeLine);
            if (alongX) g.drawLine (ref.x, pad.getY() + 4.f, ref.x, pad.getBottom() - 4.f, 1.f);
            else        g.drawLine (pad.getX() + 4.f, ref.y, pad.getRight() - 4.f, ref.y, 1.f);

            g.setColour (col.withAlpha (0.55f));
            g.drawLine ({ ref, now }, 2.5f);
        }
    }
    else if (slots > 0)
    {
        const float cell = span / (float) slots;
        for (int s = 0; s < slots; ++s)
        {
            const float a0 = (float) s / (float) slots;
            auto cellR = alongX
                ? juce::Rectangle<float> (pad.getX() + a0 * pad.getWidth(), pad.getY(),
                                          cell, pad.getHeight())
                : juce::Rectangle<float> (pad.getX(), pad.getBottom() - (a0 + 1.f / (float) slots)
                                                       * pad.getHeight(),
                                          pad.getWidth(), cell);

            // A string a finger is standing on and one that is merely still
            // ringing are different states, and each takes the colour of the
            // lane responsible - with two fingers on two engines, "which of
            // us is on this string" is the whole question the display exists
            // to answer.
            int underLane = -1, ringLane = -1;
            for (int i = 0; i < gesture.laneCount(); ++i)
            {
                const auto L = gesture.laneState (i);
                if (L.active && L.index == s) underLane = i;
                if (! gesture.cfg.pattern && s < numNotes
                    && proc.engine.strumRinging (i, notes[s]))
                    ringLane = i;
            }

            if (underLane >= 0 || ringLane >= 0)
            {
                const int which = underLane >= 0 ? underLane : ringLane;
                g.setColour (modelColour (gesture.laneState (which).model)
                                 .withAlpha (underLane >= 0 ? 0.20f : 0.09f));
                g.fillRect (cellR.reduced (0.5f));
            }

            if (s > 0)
            {
                g.setColour (ui::scopeLine);
                if (alongX) g.drawLine (cellR.getX(), pad.getY() + 3.f,
                                        cellR.getX(), pad.getBottom() - 3.f, 1.f);
                else        g.drawLine (pad.getX() + 3.f, cellR.getBottom(),
                                        pad.getRight() - 3.f, cellR.getBottom(), 1.f);
            }

            // Label every string that has room for a label. Crowding the
            // names in unreadably would be worse than leaving them out.
            const float room = alongX ? cell : (float) 26;
            if (room >= 17.f)
            {
                g.setColour (underLane >= 0
                                 ? modelColour (gesture.laneState (underLane).model)
                                 : ringLane >= 0 ? ui::inkSoft : ui::dim);
                g.setFont (ui::mono (8.f));
                const juce::String text = gesture.cfg.pattern
                    ? juce::String (s + 1)
                    : (s < numNotes ? noteName (notes[s]) : juce::String());
                g.drawText (text, cellR.reduced (1.f).withTrimmedTop (2.f).toNearestInt(),
                            juce::Justification::centredTop, false);
            }
        }
    }
    else
    {
        // Up top, clear of where a finger would be drawn.
        g.setColour (ui::dimmer);
        g.setFont (ui::sans (10.f));
        g.drawText ("hold a chord to strum",
                    pad.withHeight (34.f).toNearestInt(), juce::Justification::centred);
    }

    // The cross axis, where it is doing anything: a rail along the edge with
    // each finger's position on it.
    const int cross = gesture.cfg.cross;
    if (cross != eighty::GestureEngine::crossOff && ! ribbon)
    {
        for (int i = 0; i < gesture.laneCount(); ++i)
        {
            const auto L = gesture.laneState (i);
            if (! L.active) continue;
            const auto p = padPoint (pad, L.pos, L.cross);
            g.setColour (modelColour (L.model).withAlpha (0.4f));
            if (alongX) g.drawLine (pad.getX(), p.y, pad.getRight(), p.y, 1.f);
            else        g.drawLine (p.x, pad.getY(), p.x, pad.getBottom(), 1.f);
        }
    }

    // Fingers last, over everything.
    for (int i = 0; i < gesture.laneCount(); ++i)
    {
        const auto L = gesture.laneState (i);
        if (! L.active) continue;
        const auto p = padPoint (pad, L.pos, L.cross);
        const auto col = modelColour (L.model);

        g.setColour (col.withAlpha (0.18f));
        g.fillEllipse (p.x - 13.f, p.y - 13.f, 26.f, 26.f);
        g.setColour (col);
        g.fillEllipse (p.x - 6.f, p.y - 6.f, 12.f, 12.f);
        g.setColour (ui::scopeBg);
        g.setFont (ui::sans (8.f, true));
        g.drawText (juce::String (i + 1),
                    juce::Rectangle<float> (p.x - 6.f, p.y - 6.f, 12.f, 12.f).toNearestInt(),
                    juce::Justification::centred, false);
    }
}

void TouchMonitor::paintLane (juce::Graphics& g, juce::Rectangle<int> area, int lane,
                              const int* notes, int numNotes, int slots)
{
    const auto L = gesture.laneState (lane);
    const bool ribbon = gesture.mode() == eighty::GestureEngine::Mode::ribbon;
    const auto col = modelColour (L.model);

    g.setColour (ui::winBg);
    g.fillRoundedRectangle (area.toFloat(), 4.f);
    g.setColour (L.active ? col.withAlpha (0.55f) : ui::line);
    g.drawRoundedRectangle (area.toFloat().reduced (0.5f), 4.f, 1.f);

    auto r = area.reduced (9, 7);

    g.setColour (L.active ? ui::inkSoft : ui::dimmer);
    g.setFont (ui::sans (8.5f, true));
    g.drawText ("FINGER " + juce::String (lane + 1), r.removeFromTop (11),
                juce::Justification::centredLeft);

    // Which card this finger is driving. "AUTO" is not a hedge - it means
    // the note follows the engine mode, which is what one finger down does.
    {
        const juce::String engine = L.model == eighty::modelCS80 ? "CS-80"
                                  : L.model == eighty::modelJP8 ? "JP-8" : "AUTO";
        auto chip = area.reduced (9, 7).removeFromTop (11).removeFromRight (46);
        g.setColour (L.active ? col.withAlpha (0.22f) : ui::chipOff.withAlpha (0.5f));
        g.fillRoundedRectangle (chip.toFloat().expanded (0.f, 1.f), 2.f);
        g.setColour (L.active ? col : ui::dimmer);
        g.setFont (ui::sans (8.f, true));
        g.drawText (engine, chip, juce::Justification::centred);
    }

    r.removeFromTop (4);

    // Nothing down, or nothing to play: say so rather than draw a note
    // readout with nothing behind it.
    if (! L.active || (slots <= 0 && ! ribbon))
    {
        g.setColour (ui::dimmer);
        g.setFont (ui::sans (10.f));
        g.drawText (! L.active ? "not down" : "nothing held to strum",
                    r.removeFromTop (20), juce::Justification::centredLeft);
        return;
    }

    if (ribbon)
    {
        g.setColour (ui::scopeTrace);
        g.setFont (ui::mono (20.f));
        g.drawText ((L.semis >= 0.f ? "+" : "") + juce::String (L.semis, 1) + " st",
                    r.removeFromTop (24), juce::Justification::centredLeft);
        g.setColour (ui::dim);
        g.setFont (ui::sans (8.5f));
        g.drawText (L.model == -1 ? "bending both engines" : "bending this engine only",
                    r.removeFromTop (12), juce::Justification::centredLeft);
        return;
    }

    if (gesture.cfg.pattern)
    {
        g.setColour (ui::scopeTrace);
        g.setFont (ui::mono (20.f));
        g.drawText ("STEP " + juce::String (L.index + 1), r.removeFromTop (24),
                    juce::Justification::centredLeft);
        g.setColour (ui::dim);
        g.setFont (ui::sans (8.5f));
        g.drawText (proc.engine.es.anyArp() ? "clocking the arpeggiator"
                                         : "clocking the sequencer",
                    r.removeFromTop (12), juce::Justification::centredLeft);
        return;
    }

    // Note + velocity: the two things a stroke actually decided.
    {
        auto row = r.removeFromTop (24);
        g.setColour (ui::scopeTrace);
        g.setFont (ui::mono (20.f));
        g.drawText (L.note >= 0 ? noteName (L.note) : "-",
                    row.removeFromLeft (58), juce::Justification::centredLeft);

        auto meter = row.reduced (0, 8).withTrimmedLeft (2);
        g.setColour (ui::groove);
        g.fillRoundedRectangle (meter.toFloat(), 2.f);
        g.setColour (col);
        g.fillRoundedRectangle (meter.toFloat().withWidth (
            juce::jmax (2.f, (float) meter.getWidth() * L.vel)), 2.f);
        g.setColour (ui::dim);
        g.setFont (ui::mono (8.f));
        g.drawText ("VEL " + juce::String (L.vel, 2), row, juce::Justification::centredRight);
    }

    g.setColour (ui::dim);
    g.setFont (ui::mono (8.f));
    g.drawText ("STRING " + juce::String (L.index + 1) + "/" + juce::String (slots),
                r.removeFromTop (11), juce::Justification::centredLeft);

    // One dot per string, filled where this lane is still holding it - the
    // picture of what Ring versus Damp is actually doing. Plucked strings are
    // never held, so the dots would all be empty and say nothing.
    if (proc.engine.es.strumPluck)
    {
        r.removeFromTop (3);
        g.setColour (ui::dimmer);
        g.setFont (ui::sans (8.5f));
        g.drawText ("plucked - the patch's release shapes the decay",
                    r.removeFromTop (12), juce::Justification::centredLeft);
    }
    else
    {
        auto row = r.removeFromTop (12);
        const int n = juce::jmin (numNotes, 24);
        const float step = n > 0 ? juce::jmin (9.f, (float) row.getWidth() / (float) n) : 0.f;
        for (int s = 0; s < n; ++s)
        {
            const bool on = proc.engine.strumRinging (lane, notes[s]);
            const float x = (float) row.getX() + (float) s * step;
            g.setColour (on ? col : ui::track);
            if (on) g.fillEllipse (x, (float) row.getCentreY() - 2.5f, 5.f, 5.f);
            else    g.drawEllipse (x + 0.5f, (float) row.getCentreY() - 2.f, 4.f, 4.f, 1.f);
        }
        r.removeFromTop (3);
        g.setColour (ui::dimmer);
        g.setFont (ui::sans (7.5f));
        g.drawText (juce::String (proc.engine.strumRingCount (lane)) + " ringing",
                    row.withY (r.getY()).withHeight (10), juce::Justification::centredLeft);
    }

    // The cross axis, named and valued: the axis you are not strumming along
    // is doing something, and there is nowhere else that says what.
    {
        r.removeFromTop (12);
        juce::String text;
        switch (gesture.cfg.cross)
        {
            case eighty::GestureEngine::crossVelocity:
                text = "CROSS . VEL x" + juce::String (0.35f + 0.65f * L.cross, 2); break;
            case eighty::GestureEngine::crossBright:
            {
                const float v = L.cross * 2.f - 1.f;
                text = "CROSS . BRIGHT " + juce::String (v >= 0.f ? "+" : "")
                     + juce::String (v, 2);
                break;
            }
            case eighty::GestureEngine::crossPressure:
                text = "CROSS . PRESSURE "
                     + juce::String (gesture.lastPressure(), 2); break;
            default: text = "CROSS . off"; break;
        }
        g.setColour (ui::dim);
        g.setFont (ui::mono (8.f));
        g.drawText (text, r.removeFromTop (11), juce::Justification::centredLeft);
    }
}

void TouchMonitor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    g.setColour (ui::winBg.withAlpha (0.93f));
    g.fillRoundedRectangle (bounds.toFloat(), 8.f);
    g.setColour (ui::ledOn.withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 8.f, 1.5f);

    auto r = bounds.reduced (12);

    // ---- title row
    {
        auto row = r.removeFromTop (14);
        auto f = ui::sans (11.f, true);
        f.setExtraKerningFactor (0.05f);
        g.setColour (ui::textHi);
        g.setFont (f);
        g.drawText ("TOUCH MONITOR", row.removeFromLeft (140), juce::Justification::centredLeft);

        const bool ribbon = gesture.mode() == eighty::GestureEngine::Mode::ribbon;
        juce::String mode = ribbon ? "RIBBON"
                          : gesture.cfg.pattern ? "STRUM . PATTERN" : "STRUM . CHORD";
        if (! ribbon && ! gesture.cfg.pattern)
            mode += proc.engine.es.strumPluck ? " . PLUCK" : " . HOLD";
        if (gesture.cfg.fingers == 0 && ! ribbon) mode += "   2 FINGERS = 2 ENGINES";
        g.setColour (ui::dim);
        g.setFont (ui::sans (8.5f, true));
        g.drawText (mode, row, juce::Justification::centredRight);
    }
    g.setColour (ui::stTouch);
    g.fillRoundedRectangle ((float) r.getX(), (float) r.getY() + 1.f, 26.f, 3.f, 1.5f);
    r.removeFromTop (10);

    // The chord the strum is laid out over, read from the same published set
    // the strum itself uses.
    int notes[128];
    const int numNotes = proc.engine.strumChord (notes, 128, gesture.cfg.span);
    const int slots = gesture.slots (numNotes);

    // ---- pad on the left, at a real trackpad's 1.6:1; lanes on the right
    auto padArea = r.removeFromLeft (312);
    paintPad (g, padArea.reduced (0, 0).toFloat(), notes, numNotes, slots);

    r.removeFromLeft (12);
    const int laneH = (r.getHeight() - 8) / 2;
    paintLane (g, r.removeFromTop (laneH), 0, notes, numNotes, slots);
    r.removeFromTop (8);
    paintLane (g, r.removeFromTop (laneH), 1, notes, numNotes, slots);
}

// The settings menu. Everything in here is an ordinary parameter, so it
// saves with the session and travels in a preset like the rest of the
// instrument - a patch you strum wants its strum set up the way you left it.
void EightyEditor::showSettingsMenu()
{
    auto set = [this] (const char* id, float value)
    {
        if (auto* p = proc.apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
            paramTouched (id);
        }
    };
    auto get = [this] (const char* id) { return gestureParam (id); };

    // Menu ids index a list of closures rather than encoding a parameter and
    // a value, which keeps each row's meaning next to the row.
    auto acts = std::make_shared<std::vector<std::function<void()>>>();
    auto item = [&acts] (juce::PopupMenu& menu, const juce::String& text, bool ticked,
                         std::function<void()> action, bool enabled = true)
    {
        acts->push_back (std::move (action));
        menu.addItem ((int) acts->size(), text, enabled, ticked);
    };
    auto toggle = [&] (juce::PopupMenu& menu, const juce::String& text, const char* id)
    {
        const bool on = get (id) > 0.5f;
        item (menu, text, on, [set, id, on] { set (id, on ? 0.f : 1.f); });
    };
    // One submenu row per choice, ticked at the current one.
    auto choices = [&] (juce::PopupMenu& menu, const juce::String& label, const char* id,
                        juce::StringArray labels, bool enabled = true)
    {
        juce::PopupMenu sub;
        const int cur = (int) get (id);
        for (int i = 0; i < labels.size(); ++i)
            item (sub, labels[i], i == cur, [set, id, i] { set (id, (float) i); });
        menu.addSubMenu (label + "  (" + labels[juce::jlimit (0, labels.size() - 1, cur)] + ")",
                         sub, enabled);
    };
    auto values = [&] (juce::PopupMenu& menu, const juce::String& label, const char* id,
                       const std::vector<float>& vals, const juce::String& suffix)
    {
        juce::PopupMenu sub;
        const float cur = get (id);
        juce::String curText;
        for (float v : vals)
        {
            const bool on = std::abs (v - cur) < 0.01f;
            const bool whole = std::abs (v - std::floor (v)) < 1.0e-4f;
            const auto text = juce::String (v, whole ? 0 : 2) + suffix;
            if (on) curText = text;
            item (sub, text, on, [set, id, v] { set (id, v); });
        }
        if (curText.isEmpty()) curText = juce::String (cur, 2) + suffix;
        menu.addSubMenu (label + "  (" + curText + ")", sub);
    };

    juce::PopupMenu m;
    m.setLookAndFeel (&lnf);

    m.addSectionHeader ("Trackpad");
    if (! trackpad.isAvailable())
        m.addItem (-1, "No trackpad capture on this system", false);

    toggle (m, "Trackpad gestures", ID::tpOn);
    choices (m, "Strum key (hold)", ID::tpKey, { "Q", "Space", "` (grave)" });
    toggle (m, "Latch - the key toggles instead of holding", ID::tpLatch);
    m.addSeparator();

    m.addSectionHeader ("Strum");
    choices (m, "Strokes play", ID::tpTarget,
             { "Chord - the notes you are holding", "Pattern - clock the arp / sequencer" });
    values (m, "Span", ID::tpSpan, { 1.f, 2.f, 3.f, 4.f }, " oct");
    values (m, "Force - stroke speed to velocity", ID::tpForce,
            { 0.f, 0.25f, 0.5f, 0.6f, 0.8f, 1.f }, "");
    choices (m, "Strum axis", ID::tpAxis, { "X - across the pad", "Y - up the pad" });
    choices (m, "Cross axis", ID::tpCross,
             { "Off", "Velocity", "Brilliance", "Force Touch to aftertouch" });
    choices (m, "Articulation", ID::tpArtic,
             { "Hold - strings ring until damped",
               "Pluck - let go as the finger passes" });
    choices (m, "On lift", ID::tpLift,
             { "Ring - strings keep sounding", "Damp - let go",
               "Gesture - moving rings, resting damps" },
             gestureParam (ID::tpArtic) < 0.5f);
    choices (m, "Two fingers", ID::tpFingers,
             { "Per engine - CS-80 and JP-8 apart", "Both engines" });
    m.addSeparator();

    m.addSectionHeader ("Ribbon");
    toggle (m, "Ribbon (hold I)", ID::tpRibbon);
    values (m, "Range", ID::tpRibbonRng, { 1.f, 2.f, 5.f, 7.f, 12.f, 24.f }, " semi");
    toggle (m, "Absolute - the middle of the pad is centre pitch", ID::tpRibbonAbs);
    m.addSeparator();

    toggle (m, "Freeze the pointer while a gesture key is held", ID::tpFreeze);
    toggle (m, "Show touch monitor - where the fingers are and what they play",
            ID::tpMonitor);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (settingsBtn),
        [acts] (int result)
        {
            if (result > 0 && result <= (int) acts->size())
                (*acts)[(size_t) result - 1]();
        });
}

void EightyEditor::applyGestureSettings()
{
    auto& c = gesture.cfg;
    c.span        = (int) gestureParam (ID::tpSpan);
    c.force       = gestureParam (ID::tpForce);
    c.axis        = (int) gestureParam (ID::tpAxis);
    c.cross       = (int) gestureParam (ID::tpCross);
    c.lift        = (int) gestureParam (ID::tpLift);
    c.fingers     = (int) gestureParam (ID::tpFingers);
    c.pattern     = false;      // resolved in scanGestureKeys against what is running
    c.ribbonRange = gestureParam (ID::tpRibbonRng);
    c.ribbonMode  = gestureParam (ID::tpRibbonAbs) > 0.5f ? 1 : 0;
    trackpad.setFreezePointer (gestureParam (ID::tpFreeze) > 0.5f);
}

void EightyEditor::setGestureMode (eighty::GestureEngine::Mode m)
{
    using Mode = eighty::GestureEngine::Mode;
    if (m == gesture.mode()) return;

    gesture.setMode (m);
    trackpad.setEnabled (m != Mode::off);

    // Arming damps whatever is ringing so the hand on the pad is the only
    // thing sounding; the ribbon leaves the keyboard alone and only bends it.
    armedPattern = m == Mode::strum && gesture.cfg.pattern;
    GestureMsg g;
    g.type = GestureMsg::kArm;
    g.flag = m == Mode::strum;
    g.dir = armedPattern ? 1 : 0;
    proc.gestures.push (g);
}

// Arming is a held key, so it is scanned rather than driven by keyPressed -
// exactly as the note keys are, and for the same reason: key repeat says
// nothing about whether a key is still down.
void EightyEditor::scanGestureKeys()
{
    using Mode = eighty::GestureEngine::Mode;

    if (! trackpadAttached)
    {
        if (auto* peer = getPeer())
        {
            trackpad.attach (peer->getNativeHandle());
            trackpadAttached = true;
        }
    }

    // isKeyCurrentlyDown reads the *global* key state, so without this a Q
    // typed into some other application would arm the pad and freeze the
    // pointer out from under whatever the user was actually doing. A modal
    // dialog counts as not focused too, which is how typing a preset name
    // stays typing a preset name.
    const bool focused = juce::Process::isForegroundProcess()
                      && isShowing() && hasKeyboardFocus (true);

    if (! focused || gestureParam (ID::tpOn) < 0.5f || ! trackpad.isAvailable())
    {
        strumLatched = false;
        strumKeyWasDown = false;
        setGestureMode (Mode::off);
        if (touchMonitor != nullptr) touchMonitor->setActive (false);
        return;
    }

    static const int keyCodes[] = { 'Q', juce::KeyPress::spaceKey, '`' };
    const int code = keyCodes[juce::jlimit (0, 2, (int) gestureParam (ID::tpKey))];

    const bool strumDown = juce::KeyPress::isKeyCurrentlyDown (code);
    const bool ribbonOn = gestureParam (ID::tpRibbon) > 0.5f;
    const bool ribbonDown = ribbonOn && (juce::KeyPress::isKeyCurrentlyDown ('I')
                                      || juce::KeyPress::isKeyCurrentlyDown ('i'));

    if (gestureParam (ID::tpLatch) > 0.5f)
    {
        if (strumDown && ! strumKeyWasDown) strumLatched = ! strumLatched;
    }
    else
    {
        strumLatched = false;
    }
    strumKeyWasDown = strumDown;

    const bool strumActive = strumDown || strumLatched;

    applyGestureSettings();

    // Chord or pattern. Clocking by hand only means something when there is
    // a pattern running to clock, so it falls back to strumming the chord.
    const bool wantPattern = gestureParam (ID::tpTarget) > 0.5f
                           && (proc.engine.es.anyArp() || proc.engine.seqPlay);
    gesture.cfg.pattern = strumActive && wantPattern;

    // The ribbon wins if both keys are down: it is the finer of the two, and
    // wanting to bend a chord you are strumming is the likelier intent.
    const Mode want = ribbonDown ? Mode::ribbon
                    : strumActive ? Mode::strum : Mode::off;
    setGestureMode (want);

    // The monitor is only worth screen space while there is something on the
    // pad to watch, so it follows the arming rather than the setting alone.
    if (touchMonitor != nullptr)
        touchMonitor->setActive (want != Mode::off
                                 && gestureParam (ID::tpMonitor) > 0.5f);

    // Switching the arpeggiator or the sequencer on under a held key changes
    // what the strokes should be doing, so re-arm rather than leaving the
    // step clock in whichever state it was armed in.
    if (want == Mode::strum && gesture.cfg.pattern != armedPattern)
    {
        armedPattern = gesture.cfg.pattern;
        GestureMsg g;
        g.type = GestureMsg::kArm;
        g.flag = true;
        g.dir = armedPattern ? 1 : 0;
        proc.gestures.push (g);
    }
}

// -------------------------------------------------------- control makers
void EightyEditor::wireSlider (LearnSlider& s, juce::Label& val, const juce::String& paramID)
{
    // only assigned after the SliderAttachment exists (its initial update has run)
    s.onValueChange = [this, &s, &val, paramID]
    {
        val.setText (shortValueText (paramID, s.getValue()), juce::dontSendNotification);
        paramTouched (paramID);
    };
    val.setText (shortValueText (paramID, s.getValue()), juce::dontSendNotification);
}

VFader* EightyEditor::makeFader (Section& s, const juce::String& paramID, const juce::String& label,
                                 int width)
{
    auto* f = new VFader (label);
    owned.add (f);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        proc.apvts, paramID, f->slider));
    if (auto* param = proc.apvts.getParameter (paramID))
        f->slider.setDoubleClickReturnValue (true,
            param->convertFrom0to1 (param->getDefaultValue()));
    f->slider.onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    wireSlider (f->slider, f->value, paramID);
    const auto tip = tipFor (paramID);
    f->setTooltip (tip);
    f->slider.setTooltip (tip);
    // controls wear their section's stripe, so a row's colour says at a
    // glance which block a fader belongs to
    f->slider.setColour (juce::Slider::trackColourId, s.stripeColour());
    s.addItem (*f, width);
    controls.push_back ({ f, paramID });
    f->addMouseListener (this, true);
    return f;
}

MiniKnob* EightyEditor::makeKnob (Section& s, const juce::String& paramID, const juce::String& label,
                                  int width)
{
    auto* k = new MiniKnob (label);
    owned.add (k);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        proc.apvts, paramID, k->slider));
    if (auto* param = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true,
            param->convertFrom0to1 (param->getDefaultValue()));
    k->slider.onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    wireSlider (k->slider, k->value, paramID);
    const auto tip = tipFor (paramID);
    k->setTooltip (tip);
    k->slider.setTooltip (tip);
    k->slider.setColour (juce::Slider::trackColourId, s.stripeColour());
    s.addItem (*k, width);
    controls.push_back ({ k, paramID });
    k->addMouseListener (this, true);
    return k;
}

MiniKnob* EightyEditor::makeHeaderKnob (const juce::String& paramID, const juce::String& label)
{
    auto* k = new MiniKnob (label, true);
    k->knobSize = 28;
    owned.add (k);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        proc.apvts, paramID, k->slider));
    if (auto* param = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true,
            param->convertFrom0to1 (param->getDefaultValue()));
    k->slider.onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    wireSlider (k->slider, k->value, paramID);
    const auto tip = tipFor (paramID);
    k->setTooltip (tip);
    k->slider.setTooltip (tip);
    controls.push_back ({ k, paramID });
    k->addMouseListener (this, true);
    addAndMakeVisible (k);
    return k;
}

ChipStack* EightyEditor::makeChips (Section& s, const juce::String& paramID,
                                    juce::StringArray labels, const juce::String& group, int width)
{
    auto* param = proc.apvts.getParameter (paramID);
    jassert (param != nullptr);
    auto* c = new ChipStack (*param, std::move (labels), group, false, s.stripeColour());
    owned.add (c);
    c->setTooltip (tipFor (paramID));
    c->onUserChange = [this, paramID] { paramTouched (paramID); };
    c->onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    s.addItem (*c, width);
    controls.push_back ({ c, paramID });
    c->addMouseListener (this, true);
    return c;
}

LedToggle* EightyEditor::makeLed (Section& s, const juce::String& paramID, const juce::String& label)
{
    auto* param = proc.apvts.getParameter (paramID);
    jassert (param != nullptr);
    auto* t = new LedToggle (*param, label);
    owned.add (t);
    t->setTooltip (tipFor (paramID));
    t->onUserChange = [this, paramID] { paramTouched (paramID); };
    t->onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    s.addHeaderToggle (*t);
    controls.push_back ({ t, paramID });
    t->addMouseListener (this, true);
    return t;
}

// -------------------------------------------------------- preset bar
// ------------------------------------------------------- audio recorder
void EightyEditor::buildRecorder()
{
    recBtn.setWantsKeyboardFocus (false);
    recBtn.setClickingTogglesState (false);      // the processor owns the state
    recBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffd0402e));
    recBtn.setTooltip ("Record the output to a stereo 24-bit WAV - everything you hear, "
                       "post FX and limiter. Files land in Application Support/Eighty/"
                       "Recordings. Right-click to open the folder");
    recBtn.onClick = [this] { toggleRecording(); };
    recBtn.onRightClick = [this] { showRecorderMenu(); };
    addAndMakeVisible (recBtn);

    recTime.setJustificationType (juce::Justification::centred);
    recTime.setFont (ui::mono (9.f));
    recTime.setColour (juce::Label::textColourId, ui::dim);
    recTime.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (recTime);

    updateRecorder();
}

void EightyEditor::toggleRecording()
{
    if (proc.isRecording())
    {
        const auto gap = proc.recordingGapSamples();
        proc.stopRecording();
        const auto f = proc.lastRecording();
        readoutText = "RECORDED . " + f.getFileName().toUpperCase();
        readoutUntil = juce::Time::getMillisecondCounter() + 4000;

        // Say so rather than hand over a file with a hole in it and let it be
        // discovered later.
        if (gap > 0)
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon, "Recording dropped audio",
                "The disk could not keep up, so " + juce::String (gap)
                    + " samples are missing from\n" + f.getFileName()
                    + ".\n\nThe file is otherwise complete.");
    }
    else
    {
        juce::String error;
        if (! proc.startRecording (error))
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon, "Couldn't start recording", error);
            return;
        }
        readoutText = "RECORDING...";
        readoutUntil = juce::Time::getMillisecondCounter() + 2000;
    }
    updateRecorder();
}

void EightyEditor::showRecorderMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&lnf);
    const auto last = proc.lastRecording();
    m.addItem (1, "Reveal recordings folder in Finder");
    m.addItem (2, "Reveal last recording", last.existsAsFile());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (recBtn),
        [this, last] (int r)
        {
            if (r == 1)
            {
                auto folder = EightyProcessor::recordingsFolder();
                folder.createDirectory();
                folder.revealToUser();
            }
            else if (r == 2 && last.existsAsFile())
                last.revealToUser();
        });
}

void EightyEditor::updateRecorder()
{
    const bool on = proc.isRecording();
    recBtn.setToggleState (on, juce::dontSendNotification);
    recBtn.setButtonText (on ? "STOP" : "RECORD");

    if (on)
    {
        const int secs = (int) proc.recordedSeconds();
        recTime.setColour (juce::Label::textColourId, ui::scopeTrace);
        recTime.setText (juce::String (secs / 60).paddedLeft ('0', 2) + ":"
                       + juce::String (secs % 60).paddedLeft ('0', 2),
                         juce::dontSendNotification);
    }
    else
    {
        recTime.setColour (juce::Label::textColourId, ui::dim);
        recTime.setText (proc.lastRecording().existsAsFile() ? "SAVED" : "--:--",
                         juce::dontSendNotification);
    }
}

void EightyEditor::buildPresetBar()
{
    presetPrevBtn.setWantsKeyboardFocus (false);
    presetPrevBtn.setTooltip ("Previous preset: the built-in starting points, "
                              "then your saved files");
    presetPrevBtn.onClick = [this] { stepPreset (-1); };
    addAndMakeVisible (presetPrevBtn);

    presetNextBtn.setWantsKeyboardFocus (false);
    presetNextBtn.setTooltip ("Next preset: the built-in starting points, "
                              "then your saved files");
    presetNextBtn.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (presetNextBtn);

    presetNameBtn.setWantsKeyboardFocus (false);
    presetNameBtn.setTooltip ("Click for the preset list: built-in starting points (a bare "
                              "waveform with everything else off) above your saved presets. "
                              "An asterisk means you have edited this one since loading it");
    presetNameBtn.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (presetNameBtn);

    presetSaveBtn.setWantsKeyboardFocus (false);
    presetSaveBtn.setTooltip ("Snapshot everything - all parameters, the key map and the "
                              "sequencer pattern - into a new preset file");
    presetSaveBtn.onClick = [this] { promptSavePreset(); };
    addAndMakeVisible (presetSaveBtn);

    refreshPresetName();
}

void EightyEditor::refreshPresetName()
{
    auto name = proc.currentPresetName();
    if (proc.isPresetEdited()) name << " *";
    if (name == shownPresetName) return;
    shownPresetName = name;
    presetNameBtn.setButtonText (name);
}

void EightyEditor::applyPreset (const juce::File& file)
{
    juce::String error;
    if (! proc.loadPreset (file, error))
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::WarningIcon, "Couldn't load preset", error);
        return;
    }
    refreshPresetName();
    seqGrid.repaint();
    readoutText = "PRESET . " + proc.currentPresetName().toUpperCase();
    readoutUntil = juce::Time::getMillisecondCounter() + 2500;
    repaint();
}

std::vector<EightyEditor::PresetEntry> EightyEditor::presetEntries() const
{
    std::vector<PresetEntry> entries;
    for (int i = 0; i < EightyProcessor::getNumFactoryPresets(); ++i)
        entries.push_back ({ i, {}, EightyProcessor::getFactoryPresetName (i) });
    for (auto& f : proc.presetFiles())
        entries.push_back ({ -1, f, f.getFileNameWithoutExtension() });
    return entries;
}

void EightyEditor::applyPresetEntry (const PresetEntry& e)
{
    if (e.factoryIndex < 0) { applyPreset (e.file); return; }

    proc.loadFactoryPreset (e.factoryIndex);
    refreshPresetName();
    seqGrid.repaint();
    readoutText = "PRESET . " + proc.currentPresetName().toUpperCase();
    readoutUntil = juce::Time::getMillisecondCounter() + 2500;
    repaint();
}

void EightyEditor::stepPreset (int dir)
{
    const auto entries = presetEntries();     // never empty: factory presets
    if (entries.empty()) return;

    // Matched by name, and the factory entries come first - so a saved preset
    // that happens to share a factory name steps as the factory one. Harmless,
    // and it keeps this from needing its own notion of "where we are".
    int index = -1;
    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].name == proc.currentPresetName()) { index = i; break; }

    // not in the list (unsaved edit): step onto the first/last entry
    const int n = (int) entries.size();
    index = index < 0 ? (dir > 0 ? 0 : n - 1) : (index + dir + n) % n;
    applyPresetEntry (entries[(size_t) index]);
}

void EightyEditor::showPresetMenu()
{
    const auto entries = presetEntries();
    const int numFactory = EightyProcessor::getNumFactoryPresets();

    juce::PopupMenu m;
    m.setLookAndFeel (&lnf);
    m.addSectionHeader ("Starting points");
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        if (i == numFactory) m.addSectionHeader ("Presets");
        m.addItem (i + 1, entries[(size_t) i].name, true,
                   entries[(size_t) i].name == proc.currentPresetName());
    }
    if ((int) entries.size() == numFactory)   // nothing saved yet
    {
        m.addSectionHeader ("Presets");
        m.addItem (-1, "(none saved yet)", false);
    }
    m.addSeparator();
    m.addItem (9001, "Save as...");
    m.addItem (9002, "Reveal folder in Finder");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetNameBtn),
        [this, entries] (int result)
        {
            if (result == 9001) { promptSavePreset(); return; }
            if (result == 9002)
            {
                auto folder = EightyProcessor::presetFolder();
                folder.createDirectory();
                folder.revealToUser();
                return;
            }
            const int idx = result - 1;
            if (idx >= 0 && idx < (int) entries.size())
                applyPresetEntry (entries[(size_t) idx]);
        });
}

void EightyEditor::promptSavePreset()
{
    saveWindow = std::make_unique<juce::AlertWindow> (
        "Save preset", "Every parameter, the key map and the sequencer pattern are stored.",
        juce::MessageBoxIconType::NoIcon, this);
    saveWindow->setLookAndFeel (&lnf);
    saveWindow->addTextEditor ("name", proc.currentPresetName(), "Name:");
    saveWindow->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    saveWindow->enterModalState (true, juce::ModalCallbackFunction::create (
        [this] (int result)
        {
            std::unique_ptr<juce::AlertWindow> window (std::move (saveWindow));
            if (window != nullptr) window->setLookAndFeel (nullptr);
            if (result != 1 || window == nullptr) return;

            juce::String error;
            if (! proc.savePreset (window->getTextEditorContents ("name"), error))
            {
                juce::AlertWindow::showMessageBoxAsync (
                    juce::MessageBoxIconType::WarningIcon, "Couldn't save preset", error);
                return;
            }
            refreshPresetName();
            readoutText = "SAVED . " + proc.currentPresetName().toUpperCase();
            readoutUntil = juce::Time::getMillisecondCounter() + 2500;
        }), false);
}

LedToggle* EightyEditor::makeLooseLed (const juce::String& paramID, const juce::String& label)
{
    auto* param = proc.apvts.getParameter (paramID);
    jassert (param != nullptr);
    auto* t = new LedToggle (*param, label);
    owned.add (t);
    t->setTooltip (tipFor (paramID));
    t->onUserChange = [this, paramID] { paramTouched (paramID); };
    t->onRightClick = [this, paramID] (juce::Point<int> pos) { showLearnMenu (paramID, pos); };
    controls.push_back ({ t, paramID });
    t->addMouseListener (this, true);
    addAndMakeVisible (t);
    return t;
}

ChipStack* EightyEditor::makeLooseChips (const juce::String& paramID, juce::StringArray labels,
                                         const juce::String& group, int cellW)
{
    auto* param = proc.apvts.getParameter (paramID);
    jassert (param != nullptr);
    auto* c = new ChipStack (*param, std::move (labels), group, true, ui::stFilt);
    c->cellW = cellW;
    owned.add (c);
    c->setTooltip (tipFor (paramID));
    c->onUserChange = [this, paramID] { paramTouched (paramID); };
    c->onRightClick = [this, paramID] (juce::Point<int> pos) { showLearnMenu (paramID, pos); };
    controls.push_back ({ c, paramID });
    c->addMouseListener (this, true);
    addAndMakeVisible (c);
    return c;
}

VFader* EightyEditor::makeLooseFader (const juce::String& paramID, const juce::String& label)
{
    auto* f = new VFader (label);
    owned.add (f);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        proc.apvts, paramID, f->slider));
    if (auto* param = proc.apvts.getParameter (paramID))
        f->slider.setDoubleClickReturnValue (true,
            param->convertFrom0to1 (param->getDefaultValue()));
    f->slider.onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    wireSlider (f->slider, f->value, paramID);
    const auto tip = tipFor (paramID);
    f->setTooltip (tip);
    f->slider.setTooltip (tip);
    controls.push_back ({ f, paramID });
    f->addMouseListener (this, true);
    addAndMakeVisible (f);
    return f;
}

MiniKnob* EightyEditor::makeLooseKnob (const juce::String& paramID, const juce::String& label)
{
    auto* k = new MiniKnob (label);
    owned.add (k);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        proc.apvts, paramID, k->slider));
    if (auto* param = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true,
            param->convertFrom0to1 (param->getDefaultValue()));
    k->slider.onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    wireSlider (k->slider, k->value, paramID);
    const auto tip = tipFor (paramID);
    k->setTooltip (tip);
    k->slider.setTooltip (tip);
    controls.push_back ({ k, paramID });
    k->addMouseListener (this, true);
    addAndMakeVisible (k);
    return k;
}

// ---- sequencer row: transport, tempo, per-track strip and the step grid
void EightyEditor::buildSeqRow()
{
    seqRecLed  = makeLooseLed (ID::seqRec, "REC");
    seqPlayLed = makeLooseLed (ID::seqPlay, "PLAY");
    seqTrackChips = makeLooseChips (ID::seqTrack, { "A", "B" }, {}, 26);

    // The step clock is shared with the arp, so its tempo lives here where
    // the pattern you are timing is visible.
    tempoKnob = makeLooseKnob (ID::tempo, "TEMPO");

    // Readout strip under the grid, in the LCD idiom: what key the pattern
    // is in on the left, what is actually driving the clock on the right.
    for (auto* lab : { &seqScaleLabel, &seqTempoLabel })
    {
        auto f = ui::mono (10.f);
        f.setExtraKerningFactor (0.06f);
        lab->setFont (f);
        lab->setColour (juce::Label::textColourId, ui::scopeTrace);
        lab->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*lab);
    }
    seqScaleLabel.setJustificationType (juce::Justification::centredLeft);
    seqTempoLabel.setJustificationType (juce::Justification::centredRight);

    seqMuteLed[0] = makeLooseLed (ID::seqAMute, "MUTE");
    seqMuteLed[1] = makeLooseLed (ID::seqBMute, "MUTE");
    seqEngChips[0] = makeLooseChips (ID::seqAEng, { "AUTO", "CS", "JP" }, {}, 34);
    seqEngChips[1] = makeLooseChips (ID::seqBEng, { "AUTO", "CS", "JP" }, {}, 34);

    // CLEAR / COPY act on the armed track, so they follow the A|B chips
    auto armedTrack = [this]
    { return (int) proc.apvts.getRawParameterValue (ID::seqTrack)->load(); };

    seqClearBtn.setWantsKeyboardFocus (false);
    seqClearBtn.setTooltip ("Erase every step of the armed track");
    seqClearBtn.onClick = [this, armedTrack]
    {
        const int t = armedTrack();
        proc.engine.seqClearTrack (t);
        readoutText = "SEQ TRACK " + juce::String (t == 0 ? "A" : "B") + " CLEARED";
        readoutUntil = juce::Time::getMillisecondCounter() + 2500;
        seqGrid.repaint();
    };
    addAndMakeVisible (seqClearBtn);

    seqCopyBtn.setWantsKeyboardFocus (false);
    seqCopyBtn.setTooltip ("Copy the armed track's pattern onto the other track");
    seqCopyBtn.onClick = [this, armedTrack]
    {
        const int t = armedTrack();
        proc.engine.seqCopyTrack (t, 1 - t);
        readoutText = juce::String ("SEQ ") + (t == 0 ? "A > B" : "B > A");
        readoutUntil = juce::Time::getMillisecondCounter() + 2500;
        seqGrid.repaint();
    };
    addAndMakeVisible (seqCopyBtn);

    seqGrid.onMessage = [this] (const juce::String& s)
    {
        readoutText = s;
        readoutUntil = juce::Time::getMillisecondCounter() + 2500;
        proc.markPresetEdited();
    };
    addAndMakeVisible (seqGrid);
}

// ---- output mixer: one strip per sound source, beside the plugin loader
void EightyEditor::buildMixer()
{
    // Each strip wears the colour of the source it controls, so the mixer
    // reads as the same three engines named elsewhere in the panel.
    const struct { const char* level; const char* mute; const char* solo;
                   const char* name; juce::Colour accent; } strips[kMixStrips] = {
        { ID::csLevel,    ID::csMute,    ID::csSolo,    "CS-80", ui::stOsc    },
        { ID::jpLevel,    ID::jpMute,    ID::jpSolo,    "JP-8",  ui::jpAccent },
        { ID::synthLevel, ID::synthMute, ID::synthSolo, "SYNTH", ui::stFilt   },
    };

    for (int i = 0; i < kMixStrips; ++i)
    {
        mixFader[i] = makeLooseFader (strips[i].level, strips[i].name);
        mixFader[i]->slider.setColour (juce::Slider::trackColourId, strips[i].accent);
        mixMute[i]  = makeLooseLed (strips[i].mute, "M");
        mixSolo[i]  = makeLooseLed (strips[i].solo, "S");
    }
}

// The pattern's key and the clock actually driving it. Recomputed on the
// 30 Hz timer: the pattern is edited from the grid, loaded with a preset and
// written by step record, so there is no single place to hook a change from.
void EightyEditor::updateSeqReadout()
{
    int weights[12] = {};
    int lowest = 128, strongestPc = -1, bestWeight = 0;

    for (int t = 0; t < eighty::SynthEngine::StepSeq::kTracks; ++t)
    {
        const auto& track = proc.engine.seq.tracks[(size_t) t];
        const int len = juce::jlimit (1, eighty::SynthEngine::SeqTrack::kSteps, track.length);
        for (int s = 0; s < len; ++s)
        {
            const auto& step = track.steps[(size_t) s];
            const int n = juce::jmin ((int) step.count, eighty::SynthEngine::SeqStep::kMaxNotes);
            for (int i = 0; i < n; ++i)
            {
                const int note = juce::jlimit (0, 127, (int) step.note[i]);
                ++weights[note % 12];
                lowest = juce::jmin (lowest, note);
            }
        }
    }
    for (int pc = 0; pc < 12; ++pc)
        if (weights[pc] > bestWeight) { bestWeight = weights[pc]; strongestPc = pc; }

    const auto fit = analyseScale (weights, strongestPc, lowest < 128 ? lowest % 12 : -1);

    juce::String scale ("SCALE  ");
    if (fit.name.isEmpty())
        scale << (fit.distinct == 0 ? "-  EMPTY PATTERN" : "-  ONE NOTE");
    else
    {
        scale << (fit.exact ? "" : "~") << fit.name
              << "   " << fit.distinct << (fit.distinct == 1 ? " NOTE" : " NOTES");
        if (! fit.exact)
            scale << ", " << fit.covered << " IN KEY";
    }
    seqScaleLabel.setText (scale, juce::dontSendNotification);

    static const char* divNames[] = { "1/1", "1/2", "1/4", "1/8", "1/8T",
                                      "1/16", "1/16T", "1/32" };
    const int div = juce::jlimit (0, 7,
        (int) proc.apvts.getRawParameterValue (ID::arpDiv)->load());
    const float bpm = proc.uiBpm.load();
    seqTempoLabel.setText (juce::String (divNames[div]) + " STEPS AT "
                           + juce::String (bpm, 1) + " BPM  "
                           + (proc.uiBpmFromHost.load() ? "[HOST]" : "[PANEL]"),
                           juce::dontSendNotification);
}

MiniDisplay* EightyEditor::makeDisplay (Section& s, MiniDisplay::Kind kind,
                                        std::initializer_list<const char*> paramIDs, int width)
{
    std::vector<juce::RangedAudioParameter*> ps;
    for (auto* id : paramIDs)
        ps.push_back (proc.apvts.getParameter (id));
    auto* d = new MiniDisplay (kind, std::move (ps));
    owned.add (d);
    s.setDisplay (d, width);
    return d;
}

// --------------------------------------------------------- engine views
void EightyEditor::setEngineView (bool jupiter)
{
    jpView = jupiter;
    secOsc1.setVisible (! jupiter);
    secOsc2.setVisible (! jupiter);
    secMix.setVisible (! jupiter);
    secFilter.setVisible (! jupiter);
    secFEnv.setVisible (! jupiter);
    secAEnv.setVisible (! jupiter);
    secVco1.setVisible (jupiter);
    secVco2.setVisible (jupiter);
    secJpMix.setVisible (jupiter);
    secJpFilter.setVisible (jupiter);
    secJpFEnv.setVisible (jupiter);
    secJpAEnv.setVisible (jupiter);
    secCh2.setVisible (! jupiter);      // CS-80 only
    panelChips.setJp (jupiter);

    if (selectedCtl >= 0 && selectedCtl < (int) controls.size()
        && ! controls[(size_t) selectedCtl].target->isShowing())
        selectControl (-1);
    layoutRows();
    updateHalo();
    repaint();
}

// show only the header controls that matter for the current engine mode
void EightyEditor::updateModeVisibility (int mode)
{
    splitKnob->setVisible (mode == 2);
    csLowLed->setVisible (mode == 2);
    balKnob->setVisible (mode >= 2);
    if (zoneStrip != nullptr)
    {
        zoneStrip->setMode (mode);
        zoneStrip->setVisible (mode == 2 || mode == 4);
    }

    // CS-80/JP-8 voice-mode sections: each engine's own section shows
    // whenever that engine can sound - both at once in Split/Layer/Keys.
    secVoiceCS.setVisible (mode == 0 || mode >= 2);
    secVoiceJP.setVisible (mode == 1 || mode >= 2);
    layoutRows();
}

// ------------------------------------------------- selection & arrow keys
void EightyEditor::selectControl (int index)
{
    selectedCtl = index;
    updateHalo();
}

void EightyEditor::updateHalo()
{
    if (selectedCtl < 0 || selectedCtl >= (int) controls.size()
        || ! controls[(size_t) selectedCtl].target->isShowing())
    {
        halo.setVisible (false);
        return;
    }
    auto* t = controls[(size_t) selectedCtl].target;
    halo.setBounds (getLocalArea (t, t->getLocalBounds()).expanded (3));
    halo.setVisible (true);
}

void EightyEditor::moveSelection (int dir)
{
    if (controls.empty()) return;
    const int n = (int) controls.size();
    int idx = selectedCtl;
    for (int tries = 0; tries < n; ++tries)
    {
        idx = ((idx < 0 ? (dir > 0 ? -1 : 0) : idx) + dir + n) % n;
        if (controls[(size_t) idx].target->isShowing())
        {
            selectControl (idx);
            return;
        }
    }
}

// amount is in normalised units, signed. Stepped parameters (choices,
// integer counts) snap to their own grid and carry the remainder over, so
// a slow ramp still lands on every value in turn.
void EightyEditor::adjustSelected (float amount)
{
    if (selectedCtl < 0 || selectedCtl >= (int) controls.size()) return;
    const auto& pid = controls[(size_t) selectedCtl].paramID;
    auto* p = proc.apvts.getParameter (pid);
    if (p == nullptr) return;

    const int steps = p->getNumSteps();
    const bool stepped = steps > 1 && steps <= 128;
    float delta = amount;
    if (stepped)
    {
        const float grid = 1.f / (float) (steps - 1);
        adjustCarry += amount;
        const float n = std::trunc (adjustCarry / grid);
        if (std::abs (n) < 0.5f) return;             // not a whole step yet
        adjustCarry -= n * grid;
        delta = n * grid;
    }

    const float nv = juce::jlimit (0.f, 1.f, p->getValue() + delta);
    if (std::abs (nv - p->getValue()) < 1.0e-6f) return;
    p->setValueNotifyingHost (nv);
    paramTouched (pid);
}

// Called from the 30 Hz timer while an arrow key is held. The step size
// ramps up the longer the key is down, so a tap nudges and a hold sweeps -
// and because the voices smooth their parameters at control rate, the
// sweep comes out as a glide, not a staircase.
void EightyEditor::tickAdjustRamp()
{
    const bool shift = juce::ModifierKeys::currentModifiers.isShiftDown();
    const bool up   = ! shift && juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::upKey);
    const bool down = ! shift && juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::downKey);
    const int dir = up && ! down ? 1 : (down && ! up ? -1 : 0);

    auto endGesture = [this]
    {
        if (adjustParam != nullptr) adjustParam->endChangeGesture();
        adjustParam = nullptr;
        adjustDir = 0;
        adjustHeld = 0.f;
        adjustCarry = 0.f;
    };

    if (dir == 0) { endGesture(); return; }

    if (dir != adjustDir)
    {
        endGesture();
        adjustDir = dir;
        if (selectedCtl >= 0 && selectedCtl < (int) controls.size())
            adjustParam = proc.apvts.getParameter (controls[(size_t) selectedCtl].paramID);
        if (adjustParam != nullptr) adjustParam->beginChangeGesture();
    }

    constexpr float dt = 1.f / 30.f;
    adjustHeld += dt;
    // 0.6 %/frame at first, accelerating to 3 %/frame after ~1.5 s held
    const float rate = 0.006f + 0.024f * juce::jmin (1.f, adjustHeld / 1.5f);
    adjustSelected ((float) dir * rate);
}

void EightyEditor::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) controls.size(); ++i)
    {
        auto* t = controls[(size_t) i].target;
        if (t == e.eventComponent || t->isParentOf (e.eventComponent))
        {
            selectControl (i);
            return;
        }
    }
}

void EightyEditor::paramTouched (const juce::String& paramID)
{
    if (auto* p = proc.apvts.getParameter (paramID))
    {
        readoutText = p->getName (24).toUpperCase() + " . " + p->getCurrentValueAsText();
        readoutUntil = juce::Time::getMillisecondCounter() + 2500;
    }
    proc.markPresetEdited();
}

// ------------------------------------------------------------ MIDI learn
void EightyEditor::showLearnMenu (const juce::String& paramID, juce::Point<int> screenPos)
{
    juce::PopupMenu m;
    m.setLookAndFeel (&lnf);
    m.addItem (1, "MIDI Learn");
    const int cc = proc.midiLearn.ccForParam (paramID);
    if (cc >= 0)
        m.addItem (2, "Clear mapping (CC " + juce::String (cc) + ")");
    if (proc.midiLearn.isLearning())
        m.addItem (3, "Cancel learn");

    m.showMenuAsync (juce::PopupMenu::Options()
                        .withTargetScreenArea (juce::Rectangle<int> (screenPos.x, screenPos.y, 1, 1)),
        [this, paramID] (int result)
        {
            if (result == 1) proc.midiLearn.startLearn (paramID);
            else if (result == 2) proc.midiLearn.clearParam (paramID);
            else if (result == 3) proc.midiLearn.cancelLearn();
        });
}

// --------------------------------------------------------------- painting
void EightyEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::winBg);

    // The chassis is a stack of panels on a dark page: the header, tab,
    // sequencer and footer bands each get their own surface, while the three
    // control rows sit directly on the page because their Sections are
    // already cards.
    auto panel = [&] (int y, int h)
    {
        auto r = juce::Rectangle<float> (7.f, (float) y + 2.f,
                                         (float) getWidth() - 14.f, (float) h - 4.f);
        g.setColour (ui::bandBg);
        g.fillRoundedRectangle (r, 9.f);
        g.setColour (ui::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 9.f, 1.f);
    };

    panel (0, ui::headerH);
    panel (ui::tabY, ui::tabH);

    // engine row: the panel you are editing tints its own strip, so CS and
    // JP are told apart before you read a single label
    const auto ec = jpView ? ui::jpAccent : ui::stOsc;
    g.setColour (ec.withAlpha (0.09f));
    g.fillRoundedRectangle (7.f, (float) ui::engineY + 1.f,
                            (float) getWidth() - 14.f, (float) ui::engineH - 2.f, 9.f);
    g.setColour (ec.withAlpha (0.85f));
    g.fillRoundedRectangle (7.f, (float) ui::engineY + 1.f,
                            (float) getWidth() - 14.f, 3.f, 1.5f);

    panel (ui::seqY, ui::seqH);
    panel (ui::footerY, ui::footerH);

    // sequencer row heading + stripe, in the Section idiom
    {
        auto f = ui::sans (11.f, true);
        f.setExtraKerningFactor (0.05f);
        g.setColour (ui::textHi);
        g.setFont (f);
        g.drawText ("SEQUENCER", 14, ui::seqY + 7, 200, 12, juce::Justification::centredLeft);
        g.setColour (ui::stLfo);
        g.fillRoundedRectangle (14.f, (float) ui::seqY + 21.f, 26.f, 3.f, 1.5f);

        g.setColour (ui::dim);
        g.setFont (ui::sans (8.5f, true));
        g.drawText ("ARM", 14, ui::seqY + 52, 30, 12, juce::Justification::centredLeft);
        // one caption per grid row, aligned with that track's controls
        const int row0 = ui::seqY + 10 + SeqGrid::rulerH;
        const int row1 = row0 + SeqGrid::rowH + SeqGrid::rowGap;
        g.setColour (ui::inkSoft);
        g.drawText ("TRACK A", 212, row0 + 1, 70, 10, juce::Justification::centredLeft);
        g.drawText ("TRACK B", 212, row1 + 1, 70, 10, juce::Justification::centredLeft);

        // dark panel behind the scale / step-clock readout, matching the LCD
        g.setColour (ui::scopeBg);
        g.fillRoundedRectangle (seqStripArea.toFloat(), 3.f);
        g.setColour (ui::track);
        g.drawRoundedRectangle (seqStripArea.toFloat().reduced (0.5f), 3.f, 1.f);
    }

    // mixer card in the footer, in the insert panel's idiom
    {
        auto r = mixerArea.toFloat().reduced (0.5f);
        g.setColour (ui::cardBg);
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (ui::line);
        g.drawRoundedRectangle (r, 6.f, 1.f);
        g.setColour (ui::inkSoft);
        g.setFont (ui::sans (8.5f, true));
        g.drawText ("OUTPUT MIXER", mixerArea.getX() + 10, mixerArea.getY() + 4, 120, 10,
                    juce::Justification::centredLeft);
    }

    // preset bar caption
    g.setColour (ui::dim);
    g.setFont (ui::sans (8.5f, true));
    g.drawText ("PRESET", 16 + PanelChips::totalW + 10, ui::tabY + 12, 46, 12,
                juce::Justification::centredLeft);

    // LCD readout background
    {
        auto lcd = statusLabel.getBounds().toFloat().expanded (14.f, 3.f);
        g.setColour (ui::scopeBg);
        g.fillRoundedRectangle (lcd, 4.f);
        g.setColour (ui::track);
        g.drawRoundedRectangle (lcd.reduced (0.5f), 4.f, 1.f);
    }
}

void EightyEditor::layoutRows()
{
    // sections keep their preferred width; leftover space is shared evenly
    auto placeRow = [] (const std::vector<Section*>& row, juce::Rectangle<int> area)
    {
        int total = 0;
        for (auto* s : row) total += s->preferredWidth();
        const int n = (int) row.size();
        const int extra = juce::jmax (0, area.getWidth() - total);
        int x = area.getX();
        for (int i = 0; i < n; ++i)
        {
            const int w = row[i]->preferredWidth() + extra / n + (i < extra % n ? 1 : 0);
            row[i]->setBounds (x, area.getY(), w, area.getHeight());
            x += w;
        }
    };

    const juce::Rectangle<int> engineArea (14, ui::engineY + 3, getWidth() - 28, ui::engineH - 3);
    const juce::Rectangle<int> perfArea   (14, ui::perfY + 1, getWidth() - 28, ui::perfH - 1);
    const juce::Rectangle<int> sharedArea (14, ui::sharedY + 1, getWidth() - 28, ui::sharedH - 1);

    if (jpView)
        placeRow ({ &secVco1, &secVco2, &secJpMix, &secJpFilter, &secJpFEnv, &secJpAEnv },
                  engineArea);
    else
        placeRow ({ &secOsc1, &secOsc2, &secMix, &secFilter, &secFEnv, &secAEnv },
                  engineArea);

    // Channel II belongs to the CS-80 panel; the ring modulator is offered to
    // both cards even though it comes from the CS-80, because there is no
    // musical reason to deny it to the Jupiter layer.
    std::vector<Section*> perf;
    if (! jpView) perf.push_back (&secCh2);
    perf.push_back (&secRing);
    perf.push_back (&secPerform);
    perf.push_back (&secQuality);
    perf.push_back (&secFx);
    placeRow (perf, perfArea);

    std::vector<Section*> shared { &secLfo, &secTouch };
    if (secVoiceCS.isVisible()) shared.push_back (&secVoiceCS);
    if (secVoiceJP.isVisible()) shared.push_back (&secVoiceJP);
    shared.push_back (&secGlide);
    shared.push_back (&secArp);
    placeRow (shared, sharedArea);
}

void EightyEditor::resized()
{
    // ---- header
    titleLabel.setBounds (16, 8, 150, 24);
    subLabel.setBounds (16, 33, 170, 10);

    engineChips->setBounds (160, 15, 5 * 55 + 1, 26);

    int hx = 456;
    splitKnob->setBounds (hx, 3, 44, 51);   hx += 58;
    csLowLed->setBounds (hx, 21, csLowLed->preferredWidth() + 4, 14);
    hx += csLowLed->preferredWidth() + 18;
    balKnob->setBounds (hx, 3, 44, 51);     hx += 58;
    volKnob->setBounds (hx, 3, 44, 51);     hx += 58;
    tuneKnob->setBounds (hx, 3, 44, 51);    hx += 58;
    limitKnob->setBounds (hx, 3, 44, 51);   hx += 58;
    widthKnob->setBounds (hx, 3, 44, 51);   hx += 58;

    // The vector display is square and spans the header *and* the tab band,
    // so it is big enough to actually read; the LCD shifts left to clear it.
    const int lissSize = 74;
    const int lissX = getWidth() - 16 - lissSize;
    lissajous.setBounds (lissX, 7, lissSize, lissSize);

    // Recorder block, between the master knobs and the scope. The scope is
    // the widest thing on the panel and can spare it.
    const int recW = 92;
    recBtn.setBounds (hx + 4, 9, recW, 21);
    recTime.setBounds (hx + 4, 32, recW, 14);

    const int scopeX = hx + 4 + recW + 10;
    scope.setBounds (scopeX, 7, lissX - 8 - scopeX, 42);

    // ---- tab band: panel tabs, preset bar, LCD
    panelChips.setBounds (16, ui::tabY + 6, PanelChips::totalW, 26);
    const int lcdW = 480;
    const int lcdX = lissX - 10 - lcdW + 14;
    statusLabel.setBounds (lcdX, ui::tabY + 6, lcdW - 28, 22);

    {
        int px = 16 + PanelChips::totalW + 56;          // clear of the tabs + caption
        const int py = ui::tabY + 8, ph = 20;
        presetPrevBtn.setBounds (px, py, 22, ph);       px += 25;
        // reserve SAVE + SETTINGS on the right before the name takes the rest
        const int nameW = juce::jlimit (120, 260, lcdX - 14 - 130 - 25 - px);
        presetNameBtn.setBounds (px, py, nameW, ph);    px += nameW + 3;
        presetNextBtn.setBounds (px, py, 22, ph);       px += 28;
        presetSaveBtn.setBounds (px, py, 52, ph);       px += 56;
        settingsBtn.setBounds (px, py, 68, ph);
    }

    layoutRows();

    // ---- sequencer row: transport block, per-track strip, then the grid
    {
        const int sy = ui::seqY;
        seqRecLed->setBounds (14, sy + 30, seqRecLed->preferredWidth() + 4, 14);
        seqPlayLed->setBounds (74, sy + 30, seqPlayLed->preferredWidth() + 4, 14);
        seqTrackChips->setBounds (48, sy + 50, 2 * 26 + 1, 16);
        seqClearBtn.setBounds (14, sy + 72, 52, 16);
        seqCopyBtn.setBounds (72, sy + 72, 52, 16);
        // 56 wide gives a 50px knob, and MiniKnob needs knob + 42 in height
        // for its caption and value readout to clear the bottom
        tempoKnob->setBounds (134, sy + 8, 56, 92);

        const int gridTop = sy + 10;
        const int rowY[2] = { gridTop + SeqGrid::rulerH,
                              gridTop + SeqGrid::rulerH + SeqGrid::rowH + SeqGrid::rowGap };
        for (int t = 0; t < 2; ++t)
        {
            seqMuteLed[t]->setBounds (212, rowY[t] + 14, seqMuteLed[t]->preferredWidth() + 4, 14);
            seqEngChips[t]->setBounds (272, rowY[t] + 13, 3 * 34 + 1, 16);
        }

        const int gridX = 386;
        const int gridW = getWidth() - 14 - gridX;
        seqGrid.setBounds (gridX, gridTop, gridW, SeqGrid::preferredHeight());

        // readout strip: scale on the left, step clock on the right, sharing
        // one dark panel drawn behind them
        const int stripY = gridTop + SeqGrid::preferredHeight() + 5;
        seqStripArea = { gridX, stripY, gridW, 18 };
        seqScaleLabel.setBounds (gridX + 10, stripY, gridW / 2 - 12, 18);
        seqTempoLabel.setBounds (gridX + gridW / 2, stripY, gridW / 2 - 10, 18);
    }

    // ---- footer
    const int fy = ui::footerY + 8;
    const int fb = ui::footerY + ui::footerH - 4;
    wheel.setBounds (14, fy, 44, fb - fy);
    const int insertW = 262;
    insertPanel.setBounds (getWidth() - 14 - insertW, fy, insertW, fb - fy);

    // Mixer card, immediately left of the plugin loader: three strips, each
    // a fader with M/S under it.
    const int mixW = 192, mixX = getWidth() - 14 - insertW - 8 - mixW;
    mixerArea = { mixX, fy, mixW, fb - fy };
    {
        const int colW = (mixW - 16) / kMixStrips;
        for (int i = 0; i < kMixStrips; ++i)
        {
            const int cx = mixX + 8 + i * colW;
            mixFader[i]->setBounds (cx, fy + 14, colW, 80);
            const int mw = mixMute[i]->preferredWidth();
            const int sw = mixSolo[i]->preferredWidth();
            const int left = cx + (colW - (mw + 4 + sw)) / 2;
            mixMute[i]->setBounds (left, fy + 95, mw, 16);
            mixSolo[i]->setBounds (left + mw + 4, fy + 95, sw, 16);
        }
    }

    const int kbX = 14 + 44 + 8;
    const int kbW = mixX - 8 - kbX;
    zoneStrip->setBounds (kbX, fy, kbW, 14);
    keyboard.setBounds (kbX, fy + 16, kbW, fb - (fy + 16));
    keyboard.setKeyWidth ((float) kbW / 36.f);

    // Touch monitor: centred, over the middle rows. It only appears while a
    // gesture key is held, and while it is up the panel underneath is not
    // what you are looking at - but the LCD above and the keyboard below
    // both stay clear.
    if (touchMonitor != nullptr)
        touchMonitor->setBounds ((getWidth() - TouchMonitor::prefW) / 2,
                                 ui::perfY + 14, TouchMonitor::prefW, TouchMonitor::prefH);

    hintLabel.setBounds (0, ui::hintY, getWidth(), 20);

    updateHalo();
}

// ------------------------------------------------ computer-key handling
bool EightyEditor::keyPressed (const juce::KeyPress& kp, juce::Component*)
{
    const int code = kp.getKeyCode();

    if (code == juce::KeyPress::leftKey)  { moveSelection (-1); return true; }
    if (code == juce::KeyPress::rightKey) { moveSelection (1);  return true; }
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey)
    {
        // First press nudges straight away; holding hands over to the
        // timer's ramp (tickAdjustRamp) so the value sweeps smoothly.
        if (! kp.getModifiers().isShiftDown() && adjustDir == 0)
            tickAdjustRamp();
        return true;
    }
    if (code == juce::KeyPress::escapeKey) { selectControl (-1); return true; }

    const auto c = (juce::juce_wchar) juce::CharacterFunctions::toLowerCase (
        (juce::juce_wchar) kp.getTextCharacter());
    if (juce::String (noteKeys).containsChar (c)) return true;
    if (c == 'z' || c == 'x' || c == 'c' || c == 'v' || c == 'b' || c == 'n'
        || c == 'm' || c == 'r')
        { handleActionKey (c); return true; }

    // The gesture keys are swallowed rather than acted on: they are held,
    // not pressed, so scanGestureKeys is what actually reads them. Space has
    // to be caught here or it would work the focused button instead - but
    // only when it is actually the strum key, so it stays a normal space
    // everywhere else.
    if (gestureParam (ID::tpOn) > 0.5f)
    {
        const bool spaceIsStrum = (int) gestureParam (ID::tpKey) == 1;
        if (c == 'q' || c == 'i' || c == '`'
            || (spaceIsStrum && code == juce::KeyPress::spaceKey))
        {
            scanGestureKeys();
            return true;
        }
    }
    return false;
}

bool EightyEditor::keyStateChanged (bool, juce::Component*)
{
    scanNoteKeys();
    scanGestureKeys();
    return false;
}

void EightyEditor::scanNoteKeys()
{
    const juce::String keys (noteKeys);
    for (int i = 0; i < keys.length(); ++i)
    {
        const auto c = keys[i];
        const bool down = juce::KeyPress::isKeyCurrentlyDown ((int) c)
                       || juce::KeyPress::isKeyCurrentlyDown ((int) juce::CharacterFunctions::toUpperCase (c));
        if (down != noteKeyDown[i])
        {
            noteKeyDown[i] = down;
            const int note = baseNote + i;
            if (down) proc.keyboardState.noteOn (1, note, typeVelocity);
            else      proc.keyboardState.noteOff (1, note, 0.f);
        }
    }
}

void EightyEditor::handleActionKey (juce::juce_wchar c)
{
    auto toggleParam = [this] (const char* id)
    {
        if (auto* p = proc.apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.f : 1.f);
            p->endChangeGesture();
            paramTouched (id);
        }
    };

    if (c == 'z' || c == 'x')
    {
        const juce::String keys (noteKeys);
        for (int i = 0; i < keys.length(); ++i)
            if (noteKeyDown[i])
            {
                proc.keyboardState.noteOff (1, baseNote + i, 0.f);
                noteKeyDown[i] = false;
            }
        baseNote = juce::jlimit (36, 84, baseNote + (c == 'x' ? 12 : -12));
    }
    else if (c == 'c') typeVelocity = juce::jmax (0.1f, typeVelocity - 0.1f);
    else if (c == 'v') typeVelocity = juce::jmin (1.f, typeVelocity + 0.1f);
    else if (c == 'b') toggleParam (ID::hold);
    else if (c == 'n')
    {
        // Both engines together - the per-engine split is a panel decision,
        // and the shortcut is for getting the arp in and out of the way fast.
        const bool anyOn = proc.apvts.getRawParameterValue (ID::csArpOn)->load() > 0.5f
                        || proc.apvts.getRawParameterValue (ID::jpArpOn)->load() > 0.5f;
        for (const char* id : { ID::csArpOn, ID::jpArpOn })
            if (auto* p = proc.apvts.getParameter (id))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (anyOn ? 0.f : 1.f);
                p->endChangeGesture();
                paramTouched (id);
            }
    }
    else if (c == 'm') toggleParam (ID::seqPlay);
    else if (c == 'r') toggleParam (ID::seqRec);
}

void EightyEditor::timerCallback()
{
    if (! hasKeyboardFocus (true) && isShowing()
        && juce::Component::getCurrentlyModalComponent() == nullptr
        && ! proc.midiLearn.isLearning())
        grabKeyboardFocus();

    scanNoteKeys();
    scanGestureKeys();

    const int mode = (int) proc.apvts.getRawParameterValue (ID::engineMode)->load();
    if (mode != lastEngineMode)
    {
        if (lastEngineMode >= 0 && mode < 2)
            setEngineView (mode == 1);
        updateModeVisibility (mode);
        lastEngineMode = mode;
    }
    if (zoneStrip != nullptr && zoneStrip->isVisible())
        zoneStrip->repaint();      // tracks keyboard scrolling and param changes

    tickAdjustRamp();
    refreshPresetName();           // tracks host automation and preset loads
    updateSeqReadout();
    updateRecorder();

    if (proc.engine.seqPlay || proc.engine.seqRec)
        seqGrid.repaint();         // playhead / cursor

    const bool shift = juce::ModifierKeys::currentModifiers.isShiftDown();
    const bool up   = shift && juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::upKey);
    const bool down = shift && juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::downKey);
    bendDir = up && ! down ? 1 : (down && ! up ? -1 : 0);

    const float target = (float) bendDir;
    const bool wasActive = std::abs (keyBend) > 0.003f || bendDir != 0;
    keyBend += (target - keyBend) * 0.35f;
    if (std::abs (keyBend) < 0.003f && bendDir == 0) keyBend = 0.f;

    if (! wheelActive)
    {
        if (wasActive || keyBend != 0.f)
        {
            proc.uiBend.store (keyBend);
            proc.uiBendActive.store (true);
            wheel.setExternalValue (keyBend);
        }
        else if (proc.uiBendActive.load())
        {
            proc.uiBend.store (0.f);
            proc.uiBendActive.store (false);
            proc.engine.setBendNorm (0.f);
            wheel.setExternalValue (0.f);
        }
    }

    // LCD readout: learn > live readout > selected control > defaults
    juce::String status;
    if (proc.midiLearn.isLearning())
    {
        statusLabel.setColour (juce::Label::textColourId, ui::ledOn.brighter (0.4f));
        status = "MIDI LEARN: move a control on your MIDI device...";
    }
    else
    {
        statusLabel.setColour (juce::Label::textColourId, ui::scopeTrace);
        if (proc.lastLearnedCC.load() >= 0)
            learnFlashCC = proc.lastLearnedCC.load();

        // An armed pad takes the line over: it is a mode the keyboard is
        // now in, and there is nowhere else on the panel that says so.
        if (gesture.mode() != eighty::GestureEngine::Mode::off)
        {
            const bool ribbon = gesture.mode() == eighty::GestureEngine::Mode::ribbon;
            statusLabel.setColour (juce::Label::textColourId, ui::ledOn.brighter (0.3f));
            status = ribbon ? "RIBBON . slide to bend"
                   : gesture.cfg.pattern ? "STRUM . clocking the pattern"
                                         : "STRUM . across the held notes";
            if (strumLatched && ! ribbon) status += " [LATCHED]";
        }
        else if (readoutText.isNotEmpty() && juce::Time::getMillisecondCounter() < readoutUntil)
            status = readoutText;
        else if (selectedCtl >= 0 && selectedCtl < (int) controls.size())
        {
            if (auto* p = proc.apvts.getParameter (controls[(size_t) selectedCtl].paramID))
                status = p->getName (24).toUpperCase() + " . " + p->getCurrentValueAsText();
        }
        status += "   VOICES " + juce::String (proc.activeVoices.load()) + "/16"
                + "  OCT C" + juce::String (baseNote / 12 - 1)
                + "  VEL " + juce::String (typeVelocity, 1);
        if (learnFlashCC >= 0)
            status += "  [CC " + juce::String (learnFlashCC) + " mapped]";
    }
    statusLabel.setText (status, juce::dontSendNotification);
}
