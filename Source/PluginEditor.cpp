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
        { ID::arpOn,     "Enable the arpeggiator (shortcut: N)" },
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
CreamLNF::CreamLNF()
{
    setColour (juce::ResizableWindow::backgroundColourId, ui::winBg);
    setColour (juce::Label::textColourId, ui::ink);
    setColour (juce::PopupMenu::backgroundColourId, ui::cardBg);
    setColour (juce::PopupMenu::textColourId, ui::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::ink);
    setColour (juce::PopupMenu::highlightedTextColourId, ui::cream);
    setColour (juce::TooltipWindow::backgroundColourId, ui::ink);
    setColour (juce::TooltipWindow::textColourId, ui::cream);
    setColour (juce::TooltipWindow::outlineColourId, ui::ink);
    setColour (juce::TextButton::buttonColourId, ui::cardBg);
    setColour (juce::TextButton::textColourOffId, ui::ink);
    setColour (juce::TextButton::textColourOnId, ui::cream);
    setColour (juce::TextButton::buttonOnColourId, ui::ink);
    setColour (juce::AlertWindow::backgroundColourId, ui::cardBg);
    setColour (juce::AlertWindow::textColourId, ui::ink);
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfffdfaf3));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff26221c));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, ui::keyDown);
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ui::keyDown.withAlpha (0.45f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffd5ccba));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
}

void CreamLNF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float minSliderPos, float maxSliderPos,
                                 juce::Slider::SliderStyle, juce::Slider& slider)
{
    const float cx = (float) x + (float) w * 0.5f;
    // track
    g.setColour (ui::track);
    g.fillRoundedRectangle (cx - 2.f, (float) y, 4.f, (float) h, 2.f);

    // Zero tick for bipolar faders (e.g. Filter Env, Fine tune) - marks
    // where "off"/centred actually is, since the bottom of the fader is
    // the minimum (often negative), not zero.
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
    {
        const float zeroY = minSliderPos + (float) slider.valueToProportionOfLength (0.0)
                                          * (maxSliderPos - minSliderPos);
        g.setColour (ui::inkSoft);
        g.fillRect (cx - 7.f, zeroY - 0.75f, 14.f, 1.5f);
    }

    // cap with bg-colored center stripe
    const float capW = 22.f, capH = 12.f;
    const float cy = juce::jlimit ((float) y, (float) (y + h) - capH, pos - capH * 0.5f);
    g.setColour (ui::ink);
    g.fillRoundedRectangle (cx - capW * 0.5f, cy, capW, capH, 2.f);
    g.setColour (ui::winBg);
    g.fillRect (cx - capW * 0.5f, cy + capH * 0.5f - 1.f, capW, 2.f);
}

void CreamLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float startAngle, float endAngle,
                                 juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto sq = bounds.withSizeKeepingCentre (size, size);
    const auto c = sq.getCentre();
    const float r = size * 0.5f - 1.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // value arc: amber up to the value, track colour for the rest
    if (endAngle - angle > 0.01f)
    {
        juce::Path rest;
        rest.addCentredArc (c.x, c.y, r, r, 0.f, angle, endAngle, true);
        g.setColour (ui::track);
        g.strokePath (rest, juce::PathStrokeType (3.f));
    }
    if (angle - startAngle > 0.01f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r, r, 0.f, startAngle, angle, true);
        g.setColour (ui::scopeTrace);
        g.strokePath (arc, juce::PathStrokeType (3.f));
    }

    auto face = sq.reduced (3.f);
    auto faceCol = slider.findColour (juce::Slider::rotarySliderFillColourId);
    g.setColour (faceCol.isTransparent() ? ui::cardBg : faceCol);
    g.fillEllipse (face);
    g.setColour (ui::line);
    g.drawEllipse (face.reduced (0.5f), 1.f);

    juce::Path p;
    p.addRectangle (-1.f, -size * 0.5f + size * 0.18f, 2.f, size * 0.32f);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (ui::ink);
    g.fillPath (p);
}

void CreamLNF::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                     const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down || b.getToggleState() ? ui::ink : ui::cardBg);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (over ? ui::ink : ui::track);
    g.drawRoundedRectangle (r, 3.f, 1.f);
}

juce::Font CreamLNF::getTextButtonFont (juce::TextButton&, int) { return ui::mono (10.f); }
juce::Font CreamLNF::getPopupMenuFont() { return ui::sans (13.f); }

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
    value.setColour (juce::Label::textColourId, ui::dim);
    value.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (value);
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
    slider.setColour (juce::Slider::rotarySliderFillColourId,
                      header ? ui::winBg : ui::cardBg);
    addAndMakeVisible (slider);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (ui::sans (header ? 8.5f : 10.f, true));
    label.setColour (juce::Label::textColourId, header ? ui::dim : ui::mid);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
    value.setJustificationType (juce::Justification::centred);
    value.setFont (ui::mono (header ? 9.f : 8.5f));
    value.setColour (juce::Label::textColourId, header ? ui::ink : ui::dim);
    value.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (value);
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

    if (horiz)
    {
        for (int i = 0; i < n; ++i)
        {
            auto r = juce::Rectangle<int> (i * cellW, 0, cellW + 1, getHeight()).toFloat().reduced (0.5f);
            const bool on = i == selected;
            // JP-8 chip gets the JP accent when active
            auto fill = labels[i].contains ("JP") ? ui::jpAccent : onCol;
            if (on) { g.setColour (fill); g.fillRect (r); }
            g.setColour (on ? fill : ui::track);
            g.drawRect (r, 1.f);
            g.setColour (on ? ui::cream : ui::dim);
            g.setFont (ui::sans (cellW < 40 ? 9.f : 10.f, true));
            g.drawText (labels[i], r, juce::Justification::centred);
        }
        return;
    }

    // vertical: chips joined (-1px overlap), caption at the bottom
    const int ch = chipH();
    const int top = getHeight() - 14 - (n * (ch - 1) + 1);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<int> (0, top + i * (ch - 1), getWidth(), ch).toFloat().reduced (0.5f);
        const bool on = i == selected;
        if (on) { g.setColour (onCol); g.fillRect (r); }
        g.setColour (on ? onCol : ui::track);
        g.drawRect (r, 1.f);
        g.setColour (on ? ui::winBg : ui::dim);
        g.setFont (ui::sans (9.5f, true));
        g.drawText (labels[i], r, juce::Justification::centred);
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
    return 12 + (int) ui::sans (9.5f, true).getStringWidthFloat (text);
}

void LedToggle::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    auto led = juce::Rectangle<float> (0.f, (float) b.getCentreY() - 3.5f, 7.f, 7.f);
    g.setColour (on ? ui::ledOn : ui::ledOff);
    g.fillEllipse (led);
    if (on)
    {
        g.setColour (ui::ledOn.withAlpha (0.4f));
        g.drawEllipse (led.expanded (2.f), 2.f);
    }
    g.setColour (on ? ui::ink : ui::dim);
    g.setFont (ui::sans (9.5f, true));
    g.drawText (text, b.withTrimmedLeft (11), juce::Justification::centredLeft);
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
    setInterceptsMouseClicks (false, false);
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

void MiniDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (r, 3.f);

    auto norm = [this] (size_t i)
    { return i < params.size() && params[i] != nullptr ? params[i]->getValue() : 0.f; };

    // paths are built in the mockup's 100x28 space, then scaled to fit
    juce::Path p;
    if (kind == filterKind)
    {
        const float lp = norm (0), res = norm (1);
        const float lx = 14.f + lp * 70.f;
        p.startNewSubPath (4.f, 23.f);
        p.lineTo (11.f, 11.f);
        p.lineTo (lx - 8.f, 11.f);
        p.quadraticTo (lx, 11.f - res * 16.f, lx + 3.f, 15.f);
        p.lineTo (lx + 10.f, 26.f);
    }
    else if (kind == adsrKind)
    {
        const float a = norm (0), d = norm (1), s = norm (2), rl = norm (3);
        const float x1 = 9.f + a * 22.f;
        const float x2 = x1 + 5.f + d * 24.f;
        const float ys = 4.f + (1.f - s) * 20.f;
        p.startNewSubPath (4.f, 25.f);
        p.lineTo (x1, 4.f);
        p.lineTo (x2, ys);
        p.lineTo (72.f, ys);
        p.lineTo (74.f + rl * 22.f, 25.f);
    }
    else // lfoKind
    {
        const int wave = (int) std::round (norm (0) * 4.f);
        const float x0 = 3.f, x1 = 97.f, midY = 14.f, amp = 9.f;
        const int steps = 48;
        p.startNewSubPath (x0, midY);
        float shPrev = 0.f;
        for (int i = 1; i <= steps; ++i)
        {
            const float t = (float) i / (float) steps;          // 2 cycles over t 0..1
            const float ph = std::fmod (t * 2.f, 1.f);
            const float x = x0 + t * (x1 - x0);
            float v = 0.f;
            switch (wave)
            {
                case 0: v = std::sin (t * juce::MathConstants<float>::twoPi * 2.f); break;
                case 1: v = ph < 0.5f ? ph * 4.f - 1.f : 3.f - ph * 4.f; break;
                case 2: v = ph < 0.5f ? 1.f : -1.f; break;
                case 3: v = 1.f - ph * 2.f; break;
                default:
                {
                    static const float sh[8] = { 0.7f, -0.4f, 0.9f, 0.1f, -0.8f, 0.5f, -0.2f, -0.9f };
                    v = sh[juce::jlimit (0, 7, (int) (t * 8.f))];
                    if (v != shPrev) p.lineTo (x, midY - shPrev * amp);   // squared steps
                    shPrev = v;
                    break;
                }
            }
            p.lineTo (x, midY - v * amp);
        }
    }
    p.applyTransform (juce::AffineTransform::scale ((float) getWidth() / 100.f,
                                                    (float) getHeight() / 28.f));
    g.setColour (ui::scopeTrace);
    g.strokePath (p, juce::PathStrokeType (1.8f));
}

// ================================================================ Section
Section::Section (const juce::String& n, juce::Colour s) : name (n), stripe (s) {}

void Section::paint (juce::Graphics& g)
{
    g.setColour (ui::line);
    g.fillRect (0, 0, 1, getHeight());          // hairline left border

    auto f = ui::sans (11.f, true);
    f.setExtraKerningFactor (0.05f);
    g.setColour (ui::ink);
    g.setFont (f);
    g.drawText (name, 14, 7, getWidth() - 20, 12, juce::Justification::centredLeft);
    g.setColour (stripe);
    g.fillRect (14, 21, 26, 3);
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
        (*it)->setBounds (tx, 6, w, 14);
        tx -= 12;
    }
    if (display != nullptr)
        display->setBounds (getWidth() - 12 - displayW, 4, displayW, 22);

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
    setInterceptsMouseClicks (false, false);
    tap.addView (this);
}

void ScopeComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (r, 5.f);
    g.setColour (ui::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.f, 1.f);

    auto area = r.reduced (6.f, 6.f);
    g.setColour (ui::scopeLine);
    g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

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
    g.setColour (ui::scopeTrace);
    g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));
}

// ============================================================== Lissajous
LissajousScope::LissajousScope (ScopeTap& t) : tap (t)
{
    setInterceptsMouseClicks (false, false);
    setTooltip ("Vector display: left against right, rotated so mono draws a vertical line. "
                "A wide, round shape is a wide stereo image; a horizontal line is out of phase");
    tap.addView (this);
}

void LissajousScope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (b, 5.f);
    g.setColour (ui::line);
    g.drawRoundedRectangle (b.reduced (0.5f), 5.f, 1.f);

    const auto c = b.getCentre();
    const float rad = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 5.f;

    // graticule: the two 45-degree axes plus a bounding circle
    g.setColour (ui::scopeLine);
    g.drawEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f, 1.f);
    g.drawLine (c.x, c.y - rad, c.x, c.y + rad, 1.f);
    g.drawLine (c.x - rad, c.y, c.x + rad, c.y, 1.f);

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
    g.setColour (ui::scopeTrace.withAlpha (0.75f));
    g.strokePath (p, juce::PathStrokeType (1.f));
}

// ============================================================= PitchWheel
void PitchWheel::paint (juce::Graphics& g)
{
    auto body = getLocalBounds().toFloat().withTrimmedBottom (13.f);
    body = body.withSizeKeepingCentre (26.f, body.getHeight());
    g.setColour (ui::wheelBg);
    g.fillRoundedRectangle (body, 12.f);
    g.setColour (ui::line);
    g.drawRoundedRectangle (body.reduced (0.5f), 12.f, 1.f);

    const float travel = body.getHeight() * 0.5f - 12.f;
    const float hy = body.getCentreY() - value * travel;
    auto handle = juce::Rectangle<float> (body.getX() + 3.f, hy - 6.f, body.getWidth() - 6.f, 12.f);
    g.setColour (ui::ink);
    g.fillRoundedRectangle (handle, 3.f);

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
        if (on)
        {
            g.setColour (col);
            g.fillPath (tab);
        }
        else
        {
            g.setColour (ui::track);
            g.strokePath (tab, juce::PathStrokeType (1.f));
        }

        auto f = ui::sans (11.f, true);
        f.setExtraKerningFactor (0.06f);
        const float tw = f.getStringWidthFloat (names[i]);
        const float cx = r.getCentreX() + 7.f;              // shift right for the dot
        g.setColour (on ? ui::cream : col);
        g.fillEllipse (cx - tw * 0.5f - 14.f, r.getCentreY() - 3.5f, 7.f, 7.f);
        g.setColour (on ? ui::cream : ui::dim);
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
    g.setColour (ui::cardBg);
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

        const auto col = zone == 0 ? ui::ink : zone == 1 ? ui::jpAccent : ui::stOsc;
        g.setColour (col.withAlpha (0.88f));
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
            juce::Colour fill = beyond ? ui::scopeBg.brighter (0.02f)
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
                           : beyond || swallowed ? ui::dim.withAlpha (0.55f) : ui::cream);
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
    search.setColour (juce::TextEditor::backgroundColourId, ui::winBg);
    search.setColour (juce::TextEditor::textColourId, ui::ink);
    search.setColour (juce::TextEditor::outlineColourId, ui::track);
    search.setColour (juce::TextEditor::focusedOutlineColourId, ui::ink);
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
    g.fillAll (ui::winBg);
    g.setColour (ui::ink);
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
        g.setColour (ui::ink);
        g.fillRect (0, 0, w, h);
    }
    g.setColour (selected ? ui::cream : ui::ink);
    g.setFont (ui::sans (12.5f));
    g.drawText (d.name, 8, 0, w - 110, h, juce::Justification::centredLeft, true);
    g.setColour (selected ? ui::cream.withAlpha (0.65f) : ui::dim);
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
            juce::String(), true);
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
    makeDisplay (secFilter, MiniDisplay::filterKind, { ID::lpfCutoff, ID::resonance }, 74);
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
    makeDisplay (secFEnv, MiniDisplay::adsrKind, { ID::fEnvA, ID::fEnvD, ID::fEnvS, ID::fEnvR }, 88);
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

    makeLed   (secArp, ID::arpOn, "ON");
    makeLed   (secArp, ID::hold, "HOLD");
    makeLed   (secArp, ID::arpSync, "SYNC");
    makeChips (secArp, ID::arpMode, { "UP", "DN", "UD", "RND", "PLY" }, "MODE", 44);
    makeChips (secArp, ID::arpDiv, { "1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32" }, "DIV", 44);
    // The rate itself is now TEMPO, in the sequencer row: the arp and the
    // sequencer share one step clock, so there is one place to set it.
    makeKnob  (secArp, ID::arpOctaves, "OCT", 33);
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
    setWantsKeyboardFocus (true);
    addKeyListener (this);
    startTimerHz (30);
    setSize (ui::windowW, ui::windowH);
}

EightyEditor::~EightyEditor()
{
    removeKeyListener (this);
    setLookAndFeel (nullptr);
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
    auto* c = new ChipStack (*param, std::move (labels), group);
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
    auto* c = new ChipStack (*param, std::move (labels), group, true);
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
    static const struct { const char* level; const char* mute; const char* solo;
                          const char* name; } strips[kMixStrips] = {
        { ID::csLevel,    ID::csMute,    ID::csSolo,    "CS-80" },
        { ID::jpLevel,    ID::jpMute,    ID::jpSolo,    "JP-8"  },
        { ID::synthLevel, ID::synthMute, ID::synthSolo, "SYNTH" },
    };

    for (int i = 0; i < kMixStrips; ++i)
    {
        mixFader[i] = makeLooseFader (strips[i].level, strips[i].name);
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

    // engine row: 3px top border in the engine colour over a tinted wash
    const auto ec = jpView ? ui::jpAccent : ui::stOsc;
    g.setColour (ec.withAlpha (jpView ? 0.12f : 0.06f));
    g.fillRect (0, ui::engineY, getWidth(), ui::engineH);
    g.setColour (ec);
    g.fillRect (0, ui::engineY, getWidth(), 3);

    // shared row: hairline top border
    g.setColour (ui::line);
    g.fillRect (0, ui::sharedY, getWidth(), 1);

    // sequencer row: its own faint wash so the grid reads as one block
    g.setColour (ui::stLfo.withAlpha (0.05f));
    g.fillRect (0, ui::seqY, getWidth(), ui::seqH);
    g.setColour (ui::line);
    g.fillRect (0, ui::seqY, getWidth(), 1);

    // footer top border
    g.fillRect (0, ui::footerY, getWidth(), 1);

    // sequencer row heading + stripe, in the Section idiom
    {
        auto f = ui::sans (11.f, true);
        f.setExtraKerningFactor (0.05f);
        g.setColour (ui::ink);
        g.setFont (f);
        g.drawText ("SEQUENCER", 14, ui::seqY + 7, 200, 12, juce::Justification::centredLeft);
        g.setColour (ui::stLfo);
        g.fillRect (14, ui::seqY + 21, 26, 3);

        g.setColour (ui::dim);
        g.setFont (ui::sans (8.5f, true));
        g.drawText ("ARM", 14, ui::seqY + 52, 30, 12, juce::Justification::centredLeft);
        // one caption per grid row, aligned with that track's controls
        const int row0 = ui::seqY + 10 + SeqGrid::rulerH;
        const int row1 = row0 + SeqGrid::rowH + SeqGrid::rowGap;
        g.drawText ("TRACK A", 212, row0 + 1, 70, 10, juce::Justification::centredLeft);
        g.drawText ("TRACK B", 212, row1 + 1, 70, 10, juce::Justification::centredLeft);

        // dark panel behind the scale / step-clock readout, matching the LCD
        g.setColour (ui::scopeBg);
        g.fillRoundedRectangle (seqStripArea.toFloat(), 3.f);
    }

    // mixer card in the footer, in the insert panel's idiom
    {
        auto r = mixerArea.toFloat().reduced (0.5f);
        g.setColour (ui::cardBg);
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (ui::line);
        g.drawRoundedRectangle (r, 4.f, 1.f);
        g.setColour (ui::dim);
        g.setFont (ui::sans (8.5f, true));
        g.drawText ("MIXER", mixerArea.getX() + 10, mixerArea.getY() + 4, 120, 10,
                    juce::Justification::centredLeft);
    }

    // preset bar caption
    g.setColour (ui::dim);
    g.setFont (ui::sans (8.5f, true));
    g.drawText ("PRESET", 16 + PanelChips::totalW + 10, ui::tabY + 12, 46, 12,
                juce::Justification::centredLeft);

    // LCD readout background
    g.setColour (ui::scopeBg);
    g.fillRoundedRectangle (statusLabel.getBounds().toFloat().expanded (14.f, 0.f), 3.f);
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

    const int scopeX = hx + 6;
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
        const int nameW = juce::jlimit (120, 260, lcdX - 14 - 60 - 25 - px);
        presetNameBtn.setBounds (px, py, nameW, ph);    px += nameW + 3;
        presetNextBtn.setBounds (px, py, 22, ph);       px += 28;
        presetSaveBtn.setBounds (px, py, 52, ph);
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
            const int mw = mixMute[i]->preferredWidth() + 4;
            const int sw = mixSolo[i]->preferredWidth() + 4;
            const int left = cx + (colW - (mw + 6 + sw)) / 2;
            mixMute[i]->setBounds (left, fy + 96, mw, 14);
            mixSolo[i]->setBounds (left + mw + 6, fy + 96, sw, 14);
        }
    }

    const int kbX = 14 + 44 + 8;
    const int kbW = mixX - 8 - kbX;
    zoneStrip->setBounds (kbX, fy, kbW, 14);
    keyboard.setBounds (kbX, fy + 16, kbW, fb - (fy + 16));
    keyboard.setKeyWidth ((float) kbW / 36.f);

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
    return false;
}

bool EightyEditor::keyStateChanged (bool, juce::Component*)
{
    scanNoteKeys();
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
    else if (c == 'n') toggleParam (ID::arpOn);
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

        if (readoutText.isNotEmpty() && juce::Time::getMillisecondCounter() < readoutUntil)
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
