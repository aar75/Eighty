#include "PluginEditor.h"
#include <map>

namespace ui
{
    juce::Font sans (float size, bool bold)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }
    juce::Font mono (float size)
    {
        return juce::Font (juce::FontOptions().withName ("Menlo").withHeight (size));
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
        { ID::voiceMode,   "Poly plays chords; Mono retriggers; Legato doesn't; Unison stacks voices on one note" },
        { ID::polyVoices,  "Maximum simultaneous voices" },
        { ID::unisonCount, "Voices stacked per note in Unison mode" },
        { ID::unisonDetune,"Detune spread across the unison stack" },
        { ID::stereoSpread,"Voice panning width: odd/even voice cards go left/right" },
        { ID::drift,       "Analog inconsistency: per-voice tuning, cutoff, envelope and pan tolerances" },
        { ID::glideTime,   "Portamento time between notes" },
        { ID::glideMode,   "Glide off, only on overlapping notes, or always" },
        { ID::bendRange,   "Pitch wheel range, in semitones" },
        { ID::hold,        "Latch notes after release (shortcut: B). A new chord replaces the held one" },
        { ID::arpOn,     "Enable the arpeggiator (shortcut: N)" },
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
        { ID::engineMode, "What sounds: one engine everywhere, Split by key, or Layer both engines on every note" },
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
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, ui::ink);
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, ui::stOsc.withAlpha (0.85f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ui::stOsc.withAlpha (0.35f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffd5ccba));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
}

void CreamLNF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float, float,
                                 juce::Slider::SliderStyle, juce::Slider&)
{
    const float cx = (float) x + (float) w * 0.5f;
    // track
    g.setColour (ui::track);
    g.fillRoundedRectangle (cx - 2.f, (float) y, 4.f, (float) h, 2.f);
    // cap
    const float capW = 22.f, capH = 13.f;
    const float cy = juce::jlimit ((float) y, (float) (y + h) - capH, pos - capH * 0.5f);
    g.setColour (ui::ink);
    g.fillRoundedRectangle (cx - capW * 0.5f, cy, capW, capH, 2.f);
    g.setColour (ui::cream);
    g.fillRect (cx - capW * 0.5f, cy + capH * 0.5f - 1.f, capW, 2.f);
}

void CreamLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float startAngle, float endAngle,
                                 juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto sq = bounds.withSizeKeepingCentre (size, size);
    g.setColour (ui::cardBg);
    g.fillEllipse (sq);
    g.setColour (ui::ink);
    g.drawEllipse (sq.reduced (0.75f), 1.5f);

    const float angle = startAngle + pos * (endAngle - startAngle);
    juce::Path p;
    p.addRectangle (-1.f, -size * 0.5f + 3.f, 2.f, size * 0.36f);
    p.applyTransform (juce::AffineTransform::rotation (angle)
                          .translated (sq.getCentreX(), sq.getCentreY()));
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
    label.setFont (ui::sans (9.5f, true));
    label.setColour (juce::Label::textColourId, ui::inkSoft);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
}

void VFader::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromBottom (13));
    slider.setBounds (b);
}

MiniKnob::MiniKnob (const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (ui::sans (8.5f));
    label.setColour (juce::Label::textColourId, ui::mid);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
}

void MiniKnob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromBottom (12));
    slider.setBounds (b.removeFromBottom (30).withSizeKeepingCentre (30, 30));
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
    auto b = getLocalBounds();
    const int n = labels.size();
    if (n == 0) return;

    if (horiz)
    {
        const int cw = b.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            auto r = juce::Rectangle<int> (b.getX() + i * cw, b.getY(), cw + 1, b.getHeight()).toFloat().reduced (0.5f);
            const bool on = i == selected;
            // JP-8 chip gets the JP accent when active
            auto fill = on ? (labels[i].contains ("JP") ? ui::jpAccent : onCol) : juce::Colours::transparentBlack;
            g.setColour (on ? fill : ui::winBg.withAlpha (0.f));
            if (on) g.fillRect (r);
            g.setColour (on ? fill : ui::track);
            g.drawRect (r, 1.f);
            g.setColour (on ? ui::cream : ui::dim);
            g.setFont (ui::sans (9.5f, true));
            g.drawText (labels[i], r, juce::Justification::centred);
        }
        return;
    }

    // vertical: bottom group label, chips stacked above
    auto area = b;
    auto gl = area.removeFromBottom (13);
    g.setColour (ui::dim);
    g.setFont (ui::sans (8.f, true));
    g.drawText (group, gl, juce::Justification::centred);

    const int ch = 15;
    int top = area.getBottom() - n * ch - 2;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<int> (area.getX(), top + i * ch, area.getWidth(), ch).toFloat().reduced (0.5f);
        const bool on = i == selected;
        if (on) { g.setColour (onCol); g.fillRect (r); }
        g.setColour (on ? onCol : ui::track);
        g.drawRect (r, 1.f);
        g.setColour (on ? ui::cream : ui::dim);
        g.setFont (ui::sans (8.5f, true));
        g.drawText (labels[i], r, juce::Justification::centred);
    }
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
        idx = juce::jlimit (0, n - 1, e.x * n / juce::jmax (1, getWidth()));
    else
    {
        const int ch = 15;
        const int top = getHeight() - 13 - 2 - n * ch;
        idx = juce::jlimit (0, n - 1, (e.y - top) / ch);
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
    return 16 + (int) ui::sans (8.5f, true).getStringWidthFloat (text);
}

void LedToggle::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    auto led = juce::Rectangle<float> (0.f, (float) b.getCentreY() - 3.5f, 7.f, 7.f);
    g.setColour (on ? ui::ledOn : ui::ledOff);
    g.fillEllipse (led);
    if (on)
    {
        g.setColour (ui::ledOn.withAlpha (0.35f));
        g.drawEllipse (led.expanded (2.f), 2.f);
    }
    g.setColour (on ? ui::ink : ui::dim);
    g.setFont (ui::sans (8.5f, true));
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

// ================================================================ Section
Section::Section (const juce::String& n, juce::Colour s) : name (n), stripe (s) {}

void Section::paint (juce::Graphics& g)
{
    g.setColour (ui::line);
    g.fillRect (0, 4, 1, getHeight() - 8);      // hairline left border

    g.setColour (ui::ink);
    g.setFont (ui::sans (10.f, true));
    g.drawText (name, 14, 4, getWidth() - 20, 12, juce::Justification::centredLeft);
    g.setColour (stripe);
    g.fillRect (14, 18, 26, 3);
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

int Section::preferredWidth() const
{
    int w = 14 + 12;    // left border+pad, right pad
    for (size_t i = 0; i < items.size(); ++i)
        w += items[i].width + (i > 0 ? 4 : 0);
    return w;
}

void Section::resized()
{
    // header toggles at top-right
    int tx = getWidth() - 6;
    for (auto it = headerToggles.rbegin(); it != headerToggles.rend(); ++it)
    {
        const int w = (*it)->preferredWidth();
        tx -= w;
        (*it)->setBounds (tx, 4, w, 14);
        tx -= 10;
    }
    // content, bottom-aligned
    auto b = getLocalBounds().withTrimmedTop (24).withTrimmedLeft (14)
                             .withTrimmedRight (12).withTrimmedBottom (4);
    int x = b.getX();
    for (auto& it : items)
    {
        it.comp->setBounds (x, b.getY(), it.width, b.getHeight());
        x += it.width + 4;
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
    g.setColour (ui::ink);
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
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
}

// ============================================================= PitchWheel
void PitchWheel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.f).withTrimmedBottom (12.f);
    g.setColour (juce::Colour (0xffddd5c4));
    g.fillRoundedRectangle (r, 11.f);
    g.setColour (ui::line);
    g.drawRoundedRectangle (r, 11.f, 1.f);

    const float travel = r.getHeight() * 0.5f - 12.f;
    const float hy = r.getCentreY() - value * travel;
    auto handle = juce::Rectangle<float> (r.getX() + 3.f, hy - 6.5f, r.getWidth() - 6.f, 13.f);
    g.setColour (ui::ink);
    g.fillRoundedRectangle (handle, 3.f);

    g.setColour (ui::dim);
    g.setFont (ui::sans (8.f, true));
    g.drawText ("BEND", getLocalBounds().removeFromBottom (11), juce::Justification::centred);
}

void PitchWheel::setFromMouse (const juce::MouseEvent& e)
{
    const float half = (float) getHeight() * 0.5f;
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
    const int cw = getWidth() / 2;
    const char* names[2] = { "CS-80 PANEL", "JP-8 PANEL" };
    for (int i = 0; i < 2; ++i)
    {
        auto r = juce::Rectangle<int> (i * cw, 0, cw + (i == 0 ? 1 : 0), getHeight()).toFloat().reduced (0.5f);
        const bool on = (i == 1) == jpSel;
        auto col = i == 1 ? ui::jpAccent : ui::ink;
        if (on) { g.setColour (col); g.fillRect (r); }
        g.setColour (on ? col : ui::track);
        g.drawRect (r, 1.f);
        g.setColour (on ? ui::cream : ui::dim);
        g.setFont (ui::sans (9.f, true));
        g.drawText (names[i], r, juce::Justification::centred);
    }
}

void PanelChips::mouseDown (const juce::MouseEvent& e)
{
    const bool jp = e.x > getWidth() / 2;
    if (onSelect) onSelect (jp);
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
    g.drawText ("SYNTH LAYER", 10, 5, 100, 10, juce::Justification::centredLeft);
    g.drawText ("VST3 INSERTS", 10, 42, 100, 10, juce::Justification::centredLeft);
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
    auto b = getLocalBounds().reduced (8, 4);
    b.removeFromTop (12);
    auto synthRow = b.removeFromTop (20);
    synthClearBtn.setBounds (synthRow.removeFromRight (18).reduced (1));
    synthLevel.setBounds (synthRow.removeFromRight (22));
    synthBtn.setBounds (synthRow.reduced (1));

    b.removeFromTop (14);
    const int rowH = 17;
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
        f.setExtraKerningFactor (0.28f);
        titleLabel.setFont (f);
    }
    titleLabel.setColour (juce::Label::textColourId, ui::ink);
    addAndMakeVisible (titleLabel);

    subLabel.setText ("CS-80 x JUPITER-8", juce::dontSendNotification);
    {
        auto f = ui::sans (8.f, true);
        f.setExtraKerningFactor (0.25f);
        subLabel.setFont (f);
    }
    subLabel.setColour (juce::Label::textColourId, ui::dim);
    addAndMakeVisible (subLabel);

    statusLabel.setFont (ui::mono (10.f));
    statusLabel.setColour (juce::Label::textColourId, ui::dim);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel);

    hintLabel.setText ("PLAY: A W S E D F T G Y H U J K O L P ;    Z/X octave    C/V velocity    B hold    N arp    "
                       "LEFT/RIGHT select    UP/DOWN adjust    SHIFT+UP/DOWN bend    right-click: MIDI learn",
                       juce::dontSendNotification);
    hintLabel.setFont (ui::mono (9.f));
    hintLabel.setColour (juce::Label::textColourId, ui::dim);
    hintLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (hintLabel);

    // engine mode chips
    if (auto* param = proc.apvts.getParameter (ID::engineMode))
    {
        engineChips = std::make_unique<ChipStack> (*param,
            juce::StringArray { "CS-80", "JP-8", "SPLIT", "LAYER" },
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
    balKnob  = makeHeaderKnob (ID::engineBalance, "CS/JP");
    volKnob  = makeHeaderKnob (ID::masterVol, "VOLUME");
    tuneKnob = makeHeaderKnob (ID::masterTune, "TUNE");

    // ---- shared sections
    makeChips (secLfo, ID::lfoWave, { "SIN", "TRI", "SQR", "SAW", "S&H" }, "WAVE");
    makeFader (secLfo, ID::lfoRate, "RATE");
    makeFader (secLfo, ID::lfoDelay, "DELAY");
    makeFader (secLfo, ID::lfoToPitch, "PITCH");
    makeFader (secLfo, ID::lfoToFilter, "FILT");
    makeFader (secLfo, ID::lfoToAmp, "AMP");
    addAndMakeVisible (secLfo);

    makeFader (secMix, ID::noiseLevel, "NOISE");
    makeKnob  (secMix, ID::pwmRate, "PWM RT");
    addAndMakeVisible (secMix);

    makeFader (secTouch, ID::touchRise, "RISE");
    makeFader (secTouch, ID::touchToVib, "VIB");
    makeFader (secTouch, ID::touchToBright, "BRIGHT");
    makeFader (secTouch, ID::touchToLevel, "LEVEL");
    addAndMakeVisible (secTouch);

    makeChips (secVoice, ID::voiceMode, { "POLY", "MONO", "LEG", "UNI" }, "MODE");
    makeChips (secVoice, ID::glideMode, { "OFF", "LEG", "ALW" }, "GLIDE");
    makeKnob  (secVoice, ID::polyVoices, "VOICES");
    makeKnob  (secVoice, ID::unisonCount, "UNI CNT");
    makeKnob  (secVoice, ID::unisonDetune, "DETUNE");
    makeKnob  (secVoice, ID::stereoSpread, "SPREAD");
    makeKnob  (secVoice, ID::drift, "DRIFT");
    makeKnob  (secVoice, ID::glideTime, "GLIDE");
    makeKnob  (secVoice, ID::bendRange, "BEND");
    addAndMakeVisible (secVoice);

    makeLed   (secArp, ID::arpOn, "ON");
    makeLed   (secArp, ID::hold, "HOLD");
    makeLed   (secArp, ID::arpSync, "SYNC");
    makeChips (secArp, ID::arpMode, { "UP", "DN", "UD", "RND", "PLY" }, "MODE");
    makeChips (secArp, ID::arpDiv, { "1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32" }, "DIV");
    makeKnob  (secArp, ID::arpRateHz, "RATE");
    makeKnob  (secArp, ID::arpOctaves, "OCT");
    makeKnob  (secArp, ID::arpGate, "GATE");
    addAndMakeVisible (secArp);

    makeLed  (secFx, ID::chorusOn, "CHORUS");
    makeLed  (secFx, ID::delayOn, "DELAY");
    makeLed  (secFx, ID::tremOn, "TREM");
    makeLed  (secFx, ID::delaySync, "D.SYNC");
    makeKnob (secFx, ID::chorusRate, "C.RATE");
    makeKnob (secFx, ID::chorusDepth, "C.DEP");
    makeKnob (secFx, ID::chorusMix, "C.MIX");
    makeKnob (secFx, ID::delayTime, "D.TIME");
    makeKnob (secFx, ID::delayDiv, "D.DIV");
    makeKnob (secFx, ID::delayFB, "D.FB");
    makeKnob (secFx, ID::delayMix, "D.MIX");
    makeKnob (secFx, ID::tremRate, "T.RATE");
    makeKnob (secFx, ID::tremDepth, "T.DEP");
    addAndMakeVisible (secFx);

    // ---- CS-80 sections
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

    makeFader (secFilter, ID::hpfCutoff, "HPF");
    makeFader (secFilter, ID::lpfCutoff, "LPF");
    makeFader (secFilter, ID::resonance, "RES");
    makeFader (secFilter, ID::filterEnvAmt, "ENV");
    makeKnob  (secFilter, ID::keyTrack, "KEY TRK");
    makeKnob  (secFilter, ID::filterDrive, "DRIVE");
    addAndMakeVisible (secFilter);

    makeFader (secFEnv, ID::fEnvA, "A");
    makeFader (secFEnv, ID::fEnvD, "D");
    makeFader (secFEnv, ID::fEnvS, "S");
    makeFader (secFEnv, ID::fEnvR, "R");
    makeKnob  (secFEnv, ID::velToFilter, "VEL");
    addAndMakeVisible (secFEnv);

    makeFader (secAEnv, ID::aEnvA, "A");
    makeFader (secAEnv, ID::aEnvD, "D");
    makeFader (secAEnv, ID::aEnvS, "S");
    makeFader (secAEnv, ID::aEnvR, "R");
    makeKnob  (secAEnv, ID::velToAmp, "VEL");
    addAndMakeVisible (secAEnv);

    // ---- Jupiter-8 sections
    makeChips (secVco1, ID::jpVco1Wave, { "TRI", "SAW", "PLS", "SQR" }, "WAVE");
    makeChips (secVco1, ID::jpVco1Range, { "16'", "8'", "4'", "2'" }, "RANGE");
    makeFader (secVco1, ID::jpMix, "MIX");
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
    addAndMakeVisible (secJpFEnv);

    makeFader (secJpAEnv, ID::jpAEnvA, "A");
    makeFader (secJpAEnv, ID::jpAEnvD, "D");
    makeFader (secJpAEnv, ID::jpAEnvS, "S");
    makeFader (secJpAEnv, ID::jpAEnvR, "R");
    makeKnob  (secJpAEnv, ID::velToAmp, "VEL");
    addAndMakeVisible (secJpAEnv);

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
    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (20.f);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (insertPanel);

    addChildComponent (halo);
    halo.setAlwaysOnTop (true);

    setEngineView (false);
    setWantsKeyboardFocus (true);
    addKeyListener (this);
    startTimerHz (30);
    setSize (1480, 596);
}

EightyEditor::~EightyEditor()
{
    removeKeyListener (this);
    setLookAndFeel (nullptr);
}

// -------------------------------------------------------- control makers
static void styleLearnSlider (EightyEditor& ed, LearnSlider& s,
                              juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& paramID)
{
    s.onValueChange = [&ed, paramID] { ed.paramTouched (paramID); };
    juce::ignoreUnused (apvts);
}

VFader* EightyEditor::makeFader (Section& s, const juce::String& paramID, const juce::String& label)
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
    styleLearnSlider (*this, f->slider, proc.apvts, paramID);
    const auto tip = tipFor (paramID);
    f->setTooltip (tip);
    f->slider.setTooltip (tip);
    s.addItem (*f, 24);
    controls.push_back ({ f, paramID });
    f->addMouseListener (this, true);
    return f;
}

MiniKnob* EightyEditor::makeKnob (Section& s, const juce::String& paramID, const juce::String& label)
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
    styleLearnSlider (*this, k->slider, proc.apvts, paramID);
    const auto tip = tipFor (paramID);
    k->setTooltip (tip);
    k->slider.setTooltip (tip);
    s.addItem (*k, 34);
    controls.push_back ({ k, paramID });
    k->addMouseListener (this, true);
    return k;
}

MiniKnob* EightyEditor::makeHeaderKnob (const juce::String& paramID, const juce::String& label)
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
    styleLearnSlider (*this, k->slider, proc.apvts, paramID);
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

// --------------------------------------------------------- engine views
void EightyEditor::setEngineView (bool jupiter)
{
    jpView = jupiter;
    secOsc1.setVisible (! jupiter);
    secOsc2.setVisible (! jupiter);
    secFilter.setVisible (! jupiter);
    secFEnv.setVisible (! jupiter);
    secAEnv.setVisible (! jupiter);
    secVco1.setVisible (jupiter);
    secVco2.setVisible (jupiter);
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
    g.setColour (ui::line);
    const int rowH = 176;
    g.drawHorizontalLine (72, 14.f, (float) getWidth() - 14.f);
    g.drawHorizontalLine (72 + rowH, 14.f, (float) getWidth() - 14.f);
    g.drawHorizontalLine (72 + rowH * 2, 14.f, (float) getWidth() - 14.f);
}

void EightyEditor::layoutRows()
{
    const int rowH = 176;
    auto placeRow = [] (std::vector<Section*> row, int x, int y, int h)
    {
        for (auto* s : row)
        {
            s->setBounds (x, y, s->preferredWidth(), h);
            x += s->preferredWidth();
        }
    };

    std::vector<Section*> row1;
    if (jpView)
        row1 = { &secLfo, &secVco1, &secVco2, &secMix, &secJpFilter, &secJpFEnv, &secJpAEnv };
    else
        row1 = { &secLfo, &secOsc1, &secOsc2, &secMix, &secFilter, &secFEnv, &secAEnv };
    placeRow (row1, 14, 74, rowH - 2);

    // hidden panel's sections share the same origin so switching is clean
    std::vector<Section*> other;
    if (jpView)
        other = { &secOsc1, &secOsc2, &secFilter, &secFEnv, &secAEnv };
    else
        other = { &secVco1, &secVco2, &secJpFilter, &secJpFEnv, &secJpAEnv };
    int x = 14 + secLfo.preferredWidth();
    for (auto* s : other)
    {
        s->setBounds (x, 74, s->preferredWidth(), rowH - 2);
        x += s->preferredWidth();
    }

    placeRow ({ &secTouch, &secVoice, &secArp, &secFx }, 14, 74 + rowH, rowH - 2);
}

void EightyEditor::resized()
{
    // header
    titleLabel.setBounds (16, 10, 130, 24);
    subLabel.setBounds (18, 34, 160, 12);

    engineChips->setBounds (190, 14, 4 * 54, 24);
    panelChips.setBounds (190 + 4 * 54 + 14, 14, 170, 24);

    int hx = 190 + 4 * 54 + 14 + 170 + 20;
    splitKnob->setBounds (hx, 8, 40, 46);          hx += 44;
    csLowLed->setBounds (hx, 22, csLowLed->preferredWidth() + 4, 14); hx += csLowLed->preferredWidth() + 14;
    balKnob->setBounds (hx, 8, 40, 46);            hx += 52;
    volKnob->setBounds (hx, 8, 40, 46);            hx += 44;
    tuneKnob->setBounds (hx, 8, 40, 46);

    scope.setBounds (getWidth() - 254, 8, 240, 56);
    statusLabel.setBounds (getWidth() - 620, 46, 360, 16);

    layoutRows();

    // footer
    const int fy = 74 + 176 * 2 + 4;
    const int fh = getHeight() - fy - 20;
    wheel.setBounds (14, fy, 40, fh);
    const int insertW = 280;
    keyboard.setBounds (14 + 40 + 8, fy, getWidth() - 14 * 2 - 40 - 8 - insertW - 8, fh);
    insertPanel.setBounds (getWidth() - 14 - insertW, fy, insertW, fh);
    hintLabel.setBounds (0, getHeight() - 16, getWidth(), 12);

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
        baseNote = juce::jlimit (24, 84, baseNote + (c == 'x' ? 12 : -12));
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
        lastEngineMode = mode;
    }

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

    // footer status: learn > live readout > selected control > defaults
    juce::String status;
    if (proc.midiLearn.isLearning())
    {
        statusLabel.setColour (juce::Label::textColourId, ui::ledOn);
        status = "MIDI LEARN: move a control on your MIDI device...";
    }
    else
    {
        statusLabel.setColour (juce::Label::textColourId, ui::dim);
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
