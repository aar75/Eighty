#include "PluginEditor.h"
#include <map>

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
        { ID::hpfCutoff,   "High-pass cutoff: removes lows below this frequency (12 dB/oct)" },
        { ID::lpfCutoff,   "Low-pass cutoff: removes highs above this frequency (12 dB/oct)" },
        { ID::resonance,   "Resonance peak at the low-pass cutoff" },
        { ID::filterEnvAmt,"Filter envelope sweep amount (negative inverts the sweep)" },
        { ID::keyTrack,    "Cutoff follows keyboard position" },
        { ID::filterDrive, "Input saturation, for analog warmth" },
        { ID::fEnvA, "Filter envelope attack time" },
        { ID::fEnvD, "Filter envelope decay time" },
        { ID::fEnvS, "Filter envelope sustain level" },
        { ID::fEnvR, "Filter envelope release time" },
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
        { ID::csVoiceMode,   "CS-80: Poly plays chords; Mono retriggers; Legato doesn't; Unison stacks voices on one note" },
        { ID::csPolyVoices,  "CS-80: maximum simultaneous voices" },
        { ID::csUnisonCount, "CS-80: voices stacked per note in Unison mode" },
        { ID::csUnisonDetune,"CS-80: detune spread across the unison stack" },
        { ID::jpVoiceMode,   "JP-8: Poly plays chords; Mono retriggers; Legato doesn't; Unison stacks voices on one note" },
        { ID::jpPolyVoices,  "JP-8: maximum simultaneous voices" },
        { ID::jpUnisonCount, "JP-8: voices stacked per note in Unison mode" },
        { ID::jpUnisonDetune,"JP-8: detune spread across the unison stack" },
        { ID::stereoSpread,"Voice panning width: odd/even voice cards go left/right" },
        { ID::drift,       "Analog inconsistency: per-voice tuning, cutoff, envelope and pan tolerances" },
        { ID::glideTime,   "Portamento time between notes" },
        { ID::glideMode,   "Glide off, only on overlapping notes, or always" },
        { ID::bendRange,   "Pitch wheel range, in semitones" },
        { ID::hold,        "Latch notes after release (shortcut: B). A new chord replaces the held one" },
        { ID::arpOn,     "Enable the arpeggiator (shortcut: N)" },
        { ID::seqRec,    "Record a 16-step pattern: play notes or chords, each releases into the next step" },
        { ID::seqPlay,   "Play/stop the recorded step pattern - takes over from the live keyboard while running" },
        { ID::arpMode,   "Note order. 'PLY' plays your held notes in order, like a step sequencer" },
        { ID::arpSync,   "Sync the rate to the host tempo" },
        { ID::arpRateHz, "Free-running rate, used when Sync is off" },
        { ID::arpDiv,    "Note division, used when Sync is on" },
        { ID::arpOctaves,"Repeat the pattern across extra octaves" },
        { ID::arpGate,   "Note length within each step" },
        { ID::chorusOn,    "Bucket-brigade style stereo chorus" },
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
        { ID::engineMode, "What sounds: one engine everywhere, Split by key, Layer both on every note, "
                          "or Keys: paint each key's engine on the strip above the keyboard" },
        { ID::splitPoint, "Split key: notes below and above route to different engines (Split mode)" },
        { ID::splitCsLow, "CS-80 takes the low side of the split (off = JP-8 low)" },
        { ID::engineBalance, "Balance the CS-80 and JP-8 engines (Split & Layer modes). Center = both full" },
        { ID::synthLevel, "Level of the hosted VST3 synth layer" },
        { ID::jpVco1Wave,  "VCO1 waveform" },
        { ID::jpVco1Range, "VCO1 octave range (16' lowest, 2' highest)" },
        { ID::jpPW,        "Pulse width for both VCOs (50% = square)" },
        { ID::jpPWM,       "Pulse-width modulation depth from the PWM LFO" },
        { ID::jpMix,       "Balance: VCO1 (bottom) to VCO2 (top)" },
        { ID::jpVco2Wave,  "VCO2 waveform (Noise replaces the oscillator)" },
        { ID::jpVco2Range, "VCO2 octave range (16' lowest, 2' highest)" },
        { ID::jpVco2Semi,  "Transpose VCO2, in semitones" },
        { ID::jpVco2Fine,  "Fine tune VCO2, in cents" },
        { ID::jpSync,      "Hard-sync VCO2 to VCO1: VCO2 restarts each VCO1 cycle" },
        { ID::jpXmod,      "Cross-mod: VCO2 frequency-modulates VCO1" },
        { ID::jpHpf,       "Non-resonant high-pass cutoff" },
        { ID::jpLpf,       "Low-pass cutoff" },
        { ID::jpRes,       "Low-pass resonance (bites harder in 24 dB mode)" },
        { ID::jpSlope24,   "24 dB/oct filter slope (off = 12 dB/oct)" },
        { ID::jpEnvAmt,    "Filter envelope sweep amount (negative inverts)" },
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

    if (in ({ ID::lfoRate, ID::pwmRate, ID::arpRateHz, ID::chorusRate, ID::tremRate }))
        return juce::String (v, 1) + "Hz";

    if (in ({ ID::fEnvA, ID::fEnvD, ID::fEnvR, ID::aEnvA, ID::aEnvD, ID::aEnvR,
              ID::jpFEnvA, ID::jpFEnvD, ID::jpFEnvR, ID::jpAEnvA, ID::jpAEnvD, ID::jpAEnvR,
              ID::lfoDelay, ID::touchRise, ID::glideTime, ID::delayTime }))
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

    if (in ({ ID::filterEnvAmt, ID::jpEnvAmt }))
        return (v >= 0.005f ? "+" : "") + juce::String (v, 2);

    if (in ({ ID::csPolyVoices, ID::csUnisonCount, ID::jpPolyVoices, ID::jpUnisonCount, ID::arpOctaves }))
        return juce::String ((int) std::round (v));

    return juce::String (v, 2);
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
            auto r = juce::Rectangle<int> (i * 55, 0, 56, getHeight()).toFloat().reduced (0.5f);
            const bool on = i == selected;
            // JP-8 chip gets the JP accent when active
            auto fill = labels[i].contains ("JP") ? ui::jpAccent : onCol;
            if (on) { g.setColour (fill); g.fillRect (r); }
            g.setColour (on ? fill : ui::track);
            g.drawRect (r, 1.f);
            g.setColour (on ? ui::cream : ui::dim);
            g.setFont (ui::sans (10.f, true));
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
        idx = juce::jlimit (0, n - 1, e.x / 55);
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
ScopeComponent::ScopeComponent (ScopeFifo& f) : fifo (f)
{
    history.resize (8192, 0.f);
    display.resize (512, 0.f);
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void ScopeComponent::timerCallback()
{
    float temp[2048];
    int n;
    while ((n = fifo.pull (temp, 2048)) > 0)
        for (int i = 0; i < n; ++i)
        {
            history[(size_t) writePos] = temp[i];
            writePos = (writePos + 1) & 8191;
        }
    const int windowLen = 1400;
    int start = (writePos - windowLen - 512) & 8191;
    int trig = start;
    for (int i = 0; i < 500; ++i)
    {
        int a = (start + i) & 8191, b = (start + i + 1) & 8191;
        if (history[(size_t) a] <= 0.f && history[(size_t) b] > 0.f) { trig = b; break; }
    }
    for (size_t i = 0; i < display.size(); ++i)
    {
        int idx = (trig + (int) ((float) i * (float) windowLen / (float) display.size())) & 8191;
        display[i] = history[(size_t) idx];
    }
    repaint();
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

    juce::Path p;
    const float midY = area.getCentreY();
    const float scaleY = area.getHeight() * 0.48f;
    for (size_t i = 0; i < display.size(); ++i)
    {
        const float px = area.getX() + area.getWidth() * (float) i / (float) (display.size() - 1);
        const float py = midY - juce::jlimit (-1.f, 1.f, display[i]) * scaleY;
        if (i == 0) p.startNewSubPath (px, py);
        else        p.lineTo (px, py);
    }
    g.setColour (ui::scopeTrace);
    g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));
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
    addBtn.onClick = [this] { showAddMenu (false); };
    addAndMakeVisible (addBtn);

    synthBtn.setWantsKeyboardFocus (false);
    synthBtn.setTooltip ("Load any VST3 instrument as a third layer. It follows the arp, hold and pitch wheel");
    synthBtn.onClick = [this]
    {
        if (auto* s = proc.getSynthLayer())
            openWindowFor (s);
        else
            showAddMenu (true);
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

    synthLevel.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    synthLevel.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    synthLevel.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
    synthLevel.setWantsKeyboardFocus (false);
    synthLevel.setTooltip (tipFor (ID::synthLevel));
    synthLevelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, ID::synthLevel, synthLevel);
    // hook the readout only after the attachment's initial update has run
    synthLevel.onValueChange = [touched] { if (touched) touched (ID::synthLevel); };
    addAndMakeVisible (synthLevel);

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
    synthLevel.setBounds (synthRow.removeFromRight (22));
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

void InsertPanel::showAddMenu (bool instruments)
{
    juce::PopupMenu m;
    auto types = proc.knownPlugins.getTypes();
    int id = 1;
    for (auto& t : types)
    {
        ++id;
        if (t.isInstrument != instruments || t.name == "Eighty") continue;
        m.addItem (id - 1, t.name + "  (" + t.manufacturerName + ")");
    }
    if (types.isEmpty())
        m.addItem (900001, "Scan VST3 folders...");
    else
    {
        m.addSeparator();
        m.addItem (900001, "Rescan VST3 folders...");
    }

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (
                         instruments ? synthBtn : addBtn),
        [this, types, instruments] (int result)
        {
            if (result == 0) return;
            if (result == 900001)
            {
                juce::MouseCursor::showWaitCursor();
                proc.rescanPlugins();
                juce::MouseCursor::hideWaitCursor();
                showAddMenu (instruments);
                return;
            }
            const int idx = result - 1;
            if (idx >= 0 && idx < types.size())
            {
                juce::String error;
                const bool ok = instruments
                    ? proc.setSynthLayer (types.getReference (idx), error)
                    : proc.addInsert (types.getReference (idx), error);
                if (! ok)
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::MessageBoxIconType::WarningIcon, "Couldn't load plugin", error);
            }
        });
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
      scope (p.scopeFifo),
      insertPanel (p, [this] (const juce::String& id) { paramTouched (id); }),
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
                       "B HOLD    N ARP    LEFT/RIGHT SELECT    UP/DOWN ADJUST    "
                       "SHIFT+UP/DOWN BEND    RIGHT-CLICK: MIDI LEARN",
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
    makeKnob  (secMix, ID::pwmRate, "PWM RT");
    addAndMakeVisible (secMix);

    makeFader (secFilter, ID::hpfCutoff, "HPF");
    makeFader (secFilter, ID::lpfCutoff, "LPF");
    makeFader (secFilter, ID::resonance, "RES");
    makeFader (secFilter, ID::filterEnvAmt, "ENV");
    makeKnob  (secFilter, ID::keyTrack, "KEY TRK");
    makeKnob  (secFilter, ID::filterDrive, "DRIVE");
    makeDisplay (secFilter, MiniDisplay::filterKind, { ID::lpfCutoff, ID::resonance }, 74);
    addAndMakeVisible (secFilter);

    makeFader (secFEnv, ID::fEnvA, "A");
    makeFader (secFEnv, ID::fEnvD, "D");
    makeFader (secFEnv, ID::fEnvS, "S");
    makeFader (secFEnv, ID::fEnvR, "R");
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
    makeChips (secVco2, ID::jpVco2Wave, { "TRI", "SAW", "PLS", "NSE" }, "WAVE");
    makeChips (secVco2, ID::jpVco2Range, { "16'", "8'", "4'", "2'" }, "RANGE");
    makeKnob  (secVco2, ID::jpVco2Semi, "SEMI");
    makeKnob  (secVco2, ID::jpVco2Fine, "FINE");
    makeKnob  (secVco2, ID::jpXmod, "X-MOD");
    addAndMakeVisible (secVco2);

    makeFader (secJpMix, ID::jpMix, "MIX");
    makeKnob  (secJpMix, ID::pwmRate, "PWM RT");
    addAndMakeVisible (secJpMix);

    makeLed   (secJpFilter, ID::jpSlope24, "24dB");
    makeFader (secJpFilter, ID::jpHpf, "HPF");
    makeFader (secJpFilter, ID::jpLpf, "LPF");
    makeFader (secJpFilter, ID::jpRes, "RES");
    makeFader (secJpFilter, ID::jpEnvAmt, "ENV");
    makeKnob  (secJpFilter, ID::jpKeyTrk, "KEY TRK");
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

    makeChips (secVoiceCS, ID::csVoiceMode, { "POLY", "MONO", "LEG", "UNI" }, "MODE", 44);
    makeKnob  (secVoiceCS, ID::csPolyVoices, "VOICES", 36);
    makeKnob  (secVoiceCS, ID::csUnisonCount, "UNI CNT", 36);
    makeKnob  (secVoiceCS, ID::csUnisonDetune, "DETUNE", 36);
    addAndMakeVisible (secVoiceCS);

    makeChips (secVoiceJP, ID::jpVoiceMode, { "POLY", "MONO", "LEG", "UNI" }, "MODE", 44);
    makeKnob  (secVoiceJP, ID::jpPolyVoices, "VOICES", 36);
    makeKnob  (secVoiceJP, ID::jpUnisonCount, "UNI CNT", 36);
    makeKnob  (secVoiceJP, ID::jpUnisonDetune, "DETUNE", 36);
    addAndMakeVisible (secVoiceJP);

    makeChips (secGlide, ID::glideMode, { "OFF", "LEG", "ALW" }, "GLIDE", 44);
    makeKnob  (secGlide, ID::stereoSpread, "SPREAD", 36);
    makeKnob  (secGlide, ID::drift, "DRIFT", 36);
    makeKnob  (secGlide, ID::glideTime, "GLIDE", 36);
    makeKnob  (secGlide, ID::bendRange, "BEND", 36);
    addAndMakeVisible (secGlide);

    makeLed   (secArp, ID::arpOn, "ON");
    makeLed   (secArp, ID::seqRec, "REC");
    makeLed   (secArp, ID::seqPlay, "PLAY");
    makeLed   (secArp, ID::hold, "HOLD");
    makeLed   (secArp, ID::arpSync, "SYNC");
    makeChips (secArp, ID::arpMode, { "UP", "DN", "UD", "RND", "PLY" }, "MODE", 44);
    makeChips (secArp, ID::arpDiv, { "1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32" }, "DIV", 44);
    makeKnob  (secArp, ID::arpRateHz, "RATE", 36);
    makeKnob  (secArp, ID::arpOctaves, "OCT", 36);
    makeKnob  (secArp, ID::arpGate, "GATE", 36);
    addAndMakeVisible (secArp);

    makeLed  (secFx, ID::chorusOn, "CHORUS");
    makeLed  (secFx, ID::delayOn, "DELAY");
    makeLed  (secFx, ID::tremOn, "TREM");
    makeLed  (secFx, ID::delaySync, "D.SYNC");
    makeKnob (secFx, ID::chorusRate, "C.RATE", 36);
    makeKnob (secFx, ID::chorusDepth, "C.DEP", 36);
    makeKnob (secFx, ID::chorusMix, "C.MIX", 36);
    makeKnob (secFx, ID::delayTime, "D.TIME", 36);
    makeKnob (secFx, ID::delayDiv, "D.DIV", 36);
    makeKnob (secFx, ID::delayFB, "D.FB", 36);
    makeKnob (secFx, ID::delayMix, "D.MIX", 36);
    makeKnob (secFx, ID::tremRate, "T.RATE", 36);
    makeKnob (secFx, ID::tremDepth, "T.DEP", 36);
    addAndMakeVisible (secFx);

    addAndMakeVisible (scope);

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
    setWantsKeyboardFocus (true);
    addKeyListener (this);
    startTimerHz (30);
    setSize (1720, 596);
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

void EightyEditor::adjustSelected (int dir)
{
    if (selectedCtl < 0 || selectedCtl >= (int) controls.size()) return;
    const auto& pid = controls[(size_t) selectedCtl].paramID;
    auto* p = proc.apvts.getParameter (pid);
    if (p == nullptr) return;

    const int steps = p->getNumSteps();
    const float delta = (steps > 1 && steps <= 128) ? 1.f / (float) (steps - 1) : 0.02f;
    const float nv = juce::jlimit (0.f, 1.f, p->getValue() + (float) dir * delta);
    p->beginChangeGesture();
    p->setValueNotifyingHost (nv);
    p->endChangeGesture();
    paramTouched (pid);
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
    g.fillRect (0, 88, getWidth(), 180);
    g.setColour (ec);
    g.fillRect (0, 88, getWidth(), 3);

    // shared row: hairline top border
    g.setColour (ui::line);
    g.fillRect (0, 268, getWidth(), 1);

    // footer top border
    g.fillRect (0, 448, getWidth(), 1);

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

    const juce::Rectangle<int> engineArea (14, 91, getWidth() - 28, 177);
    const juce::Rectangle<int> sharedArea (14, 269, getWidth() - 28, 179);

    if (jpView)
        placeRow ({ &secVco1, &secVco2, &secJpMix, &secJpFilter, &secJpFEnv, &secJpAEnv },
                  engineArea);
    else
        placeRow ({ &secOsc1, &secOsc2, &secMix, &secFilter, &secFEnv, &secAEnv },
                  engineArea);

    std::vector<Section*> shared { &secLfo, &secTouch };
    if (secVoiceCS.isVisible()) shared.push_back (&secVoiceCS);
    if (secVoiceJP.isVisible()) shared.push_back (&secVoiceJP);
    shared.push_back (&secGlide);
    shared.push_back (&secArp);
    shared.push_back (&secFx);
    placeRow (shared, sharedArea);
}

void EightyEditor::resized()
{
    // ---- header (56px)
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

    scope.setBounds (hx + 6, 7, getWidth() - 16 - (hx + 6), 42);

    // ---- tab band (32px)
    panelChips.setBounds (16, 62, PanelChips::totalW, 26);
    const int lcdW = 480;
    statusLabel.setBounds (getWidth() - 16 - lcdW + 14, 62, lcdW - 28, 22);

    layoutRows();

    // ---- footer
    const int fy = 456, fb = 574;
    wheel.setBounds (14, fy, 44, fb - fy);
    const int insertW = 262;
    insertPanel.setBounds (getWidth() - 14 - insertW, fy, insertW, fb - fy);
    const int kbX = 14 + 44 + 8;
    const int kbW = getWidth() - 14 - insertW - 8 - kbX;
    zoneStrip->setBounds (kbX, fy, kbW, 14);
    keyboard.setBounds (kbX, fy + 16, kbW, fb - (fy + 16));
    keyboard.setKeyWidth ((float) kbW / 36.f);

    hintLabel.setBounds (0, 576, getWidth(), 20);

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
        if (kp.getModifiers().isShiftDown())
            return true;
        adjustSelected (code == juce::KeyPress::upKey ? 1 : -1);
        return true;
    }
    if (code == juce::KeyPress::escapeKey) { selectControl (-1); return true; }

    const auto c = (juce::juce_wchar) juce::CharacterFunctions::toLowerCase (
        (juce::juce_wchar) kp.getTextCharacter());
    if (juce::String (noteKeys).containsChar (c)) return true;
    if (c == 'z' || c == 'x' || c == 'c' || c == 'v' || c == 'b' || c == 'n')
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
