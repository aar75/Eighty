#include "PluginEditor.h"
#include <map>

// ============================================================ LookAndFeel
EightyLookAndFeel::EightyLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, ui::bg);
    setColour (juce::Slider::rotarySliderFillColourId, ui::accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, ui::panelLine);
    setColour (juce::Slider::thumbColourId, ui::text);
    setColour (juce::Slider::textBoxTextColourId, ui::text);
    setColour (juce::Label::textColourId, ui::text);
    setColour (juce::ComboBox::backgroundColourId, ui::bg);
    setColour (juce::ComboBox::textColourId, ui::text);
    setColour (juce::ComboBox::outlineColourId, ui::panelLine);
    setColour (juce::ComboBox::arrowColourId, ui::dimText);
    setColour (juce::PopupMenu::backgroundColourId, ui::panel);
    setColour (juce::PopupMenu::textColourId, ui::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, ui::text);
    setColour (juce::TextButton::buttonColourId, ui::bg);
    setColour (juce::TextButton::buttonOnColourId, ui::accent);
    setColour (juce::TextButton::textColourOffId, ui::dimText);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff17130a));
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff262a33));
    setColour (juce::TooltipWindow::textColourId, ui::text);
    setColour (juce::TooltipWindow::outlineColourId, ui::panelLine);
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd8dbe2));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff23262e));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, ui::accent.withAlpha (0.8f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ui::accent.withAlpha (0.3f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff9aa0ab));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
}

void EightyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float startAngle, float endAngle,
                                          juce::Slider& s)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto square = bounds.withSizeKeepingCentre (size, size);
    const float radius = size * 0.5f;
    const float cx = square.getCentreX(), cy = square.getCentreY();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcR = radius - 2.f;

    juce::Path track;
    track.addCentredArc (cx, cy, arcR, arcR, 0.f, startAngle, endAngle, true);
    g.setColour (s.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (cx, cy, arcR, arcR, 0.f, startAngle, angle, true);
    g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
    g.strokePath (value, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -arcR + 3.f, 3.f, arcR * 0.42f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (s.findColour (juce::Slider::thumbColourId));
    g.fillPath (pointer);
}

void EightyLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool,
                                      int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 5.f, 1.f);

    juce::Path arrow;
    const float ax = (float) w - 14.f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 3.5f, ay - 2.f, ax + 3.5f, ay - 2.f, ax, ay + 3.f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void EightyLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                          bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto pill = r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 4.f, 74.f), 22.f);
    const bool on = b.getToggleState();

    g.setColour (on ? ui::accent : ui::bg);
    g.fillRoundedRectangle (pill, 11.f);
    g.setColour (on ? ui::accent : (highlighted ? ui::dimText : ui::panelLine));
    g.drawRoundedRectangle (pill, 11.f, 1.f);
    g.setColour (on ? juce::Colour (0xff17130a) : ui::dimText);
    g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    g.drawText (b.getButtonText(), pill, juce::Justification::centred);
}

juce::Font EightyLookAndFeel::getComboBoxFont (juce::ComboBox&)  { return juce::Font (juce::FontOptions (12.f)); }
juce::Font EightyLookAndFeel::getPopupMenuFont()                 { return juce::Font (juce::FontOptions (13.f)); }
juce::Font EightyLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return juce::Font (juce::FontOptions (11.f, juce::Font::bold)); }

// =================================================================== Knob
Knob::Knob (const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (10.f)));
    label.setColour (juce::Label::textColourId, ui::dimText);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromBottom (13));
    slider.setBounds (b);
}

// ================================================================ Section
Section::Section (const juce::String& t, juce::Colour ac) : title (t), accentCol (ac) {}

void Section::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (ui::panel);
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (ui::panelLine);
    g.drawRoundedRectangle (r, 8.f, 1.f);

    g.setColour (accentCol);
    g.fillRoundedRectangle (10.f, 8.f, 3.f, 12.f, 1.5f);
    g.setColour (ui::dimText);
    g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    g.drawText (title, 19, 6, getWidth() - 24, 15, juce::Justification::centredLeft);
}

void Section::addItem (juce::Component& c, int span)
{
    items.push_back ({ &c, span });
    addAndMakeVisible (c);
}

void Section::resized()
{
    auto b = getLocalBounds().reduced (8);
    b.removeFromTop (16);
    if (items.empty() || b.isEmpty()) return;

    int totalRows = 1, col = 0;
    for (auto& it : items)
    {
        if (col + it.span > columns) { col = 0; ++totalRows; }
        col += it.span;
    }
    const int cellW = b.getWidth() / columns;
    const int cellH = b.getHeight() / totalRows;

    col = 0; int row = 0;
    for (auto& it : items)
    {
        if (col + it.span > columns) { col = 0; ++row; }
        it.comp->setBounds (b.getX() + col * cellW, b.getY() + row * cellH,
                            cellW * it.span, cellH);
        col += it.span;
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
    auto r = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (juce::Colour (0xff101216));
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (ui::panelLine);
    g.drawRoundedRectangle (r, 8.f, 1.f);

    g.setColour (ui::dimText);
    g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    g.drawText ("OUT", 12, 6, 60, 14, juce::Justification::centredLeft);

    auto area = getLocalBounds().toFloat().reduced (8.f, 22.f);
    g.setColour (ui::panelLine.withAlpha (0.6f));
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
    g.setColour (ui::accent.withAlpha (0.9f));
    g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
}

// ============================================================= PitchWheel
void PitchWheel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.f);
    g.setColour (juce::Colour (0xff101216));
    g.fillRoundedRectangle (r, 7.f);
    g.setColour (ui::panelLine);
    g.drawRoundedRectangle (r, 7.f, 1.f);

    const float cy = r.getCentreY();
    g.setColour (ui::panelLine);
    g.drawHorizontalLine ((int) cy, r.getX() + 4.f, r.getRight() - 4.f);

    const float travel = (r.getHeight() * 0.5f - 14.f);
    const float hy = cy - value * travel;
    auto handle = juce::Rectangle<float> (r.getX() + 3.f, hy - 9.f, r.getWidth() - 6.f, 18.f);
    g.setColour (ui::accent);
    g.fillRoundedRectangle (handle, 4.f);
    g.setColour (juce::Colour (0xff17130a));
    g.drawHorizontalLine ((int) handle.getCentreY(), handle.getX() + 3.f, handle.getRight() - 3.f);

    g.setColour (ui::dimText);
    g.setFont (juce::Font (juce::FontOptions (9.f, juce::Font::bold)));
    g.drawText ("BEND", getLocalBounds().removeFromBottom (12), juce::Justification::centred);
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
    if (std::abs (v - value) > 0.001f)
    {
        value = v;
        repaint();
    }
}

// ============================================================== tooltips
namespace
{
juce::String tipFor (const juce::String& id)
{
    static const std::map<juce::String, const char*> tips = {
        // CS-80 oscillators
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
        // CS-80 filter
        { ID::hpfCutoff,   "High-pass cutoff: removes lows below this frequency (12 dB/oct)" },
        { ID::lpfCutoff,   "Low-pass cutoff: removes highs above this frequency (12 dB/oct)" },
        { ID::resonance,   "Resonance peak at the low-pass cutoff" },
        { ID::filterEnvAmt,"Filter envelope sweep amount (negative inverts the sweep)" },
        { ID::keyTrack,    "Cutoff follows keyboard position" },
        { ID::filterDrive, "Input saturation, for analog warmth" },
        // envelopes
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
        // LFO
        { ID::lfoRate,    "LFO speed" },
        { ID::lfoWave,    "LFO shape" },
        { ID::lfoDelay,   "LFO fade-in time after the first key press" },
        { ID::lfoToPitch, "Vibrato: LFO to pitch" },
        { ID::lfoToFilter,"Filter wobble: LFO to cutoff" },
        { ID::lfoToAmp,   "Tremolo: LFO to voice level" },
        // touch
        { ID::touchRise,    "How quickly simulated key pressure builds while a key is held" },
        { ID::touchToVib,   "Pressure adds vibrato (real aftertouch works too)" },
        { ID::touchToBright,"Pressure opens the filter" },
        { ID::touchToLevel, "Pressure raises the volume" },
        // voices
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
        // arp
        { ID::arpOn,     "Enable the arpeggiator (shortcut: N)" },
        { ID::arpMode,   "Note order. 'As Played' is a step sequencer of your held notes" },
        { ID::arpSync,   "Sync the rate to the host tempo" },
        { ID::arpRateHz, "Free-running rate, used when Sync is off" },
        { ID::arpDiv,    "Note division, used when Sync is on" },
        { ID::arpOctaves,"Repeat the pattern across extra octaves" },
        { ID::arpGate,   "Note length within each step" },
        // fx
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
        // master / engine
        { ID::masterVol,  "Output volume" },
        { ID::masterTune, "Global tune, in cents" },
        { ID::engineMode, "What sounds: one engine everywhere, Split by key, or Layer both engines on every note" },
        { ID::splitPoint, "Split key: notes below and above route to different engines (Split mode)" },
        { ID::splitCsLow, "CS-80 takes the low side of the split (off = JP-8 low)" },
        // Jupiter-8
        { ID::jpVco1Wave,  "VCO1 waveform" },
        { ID::jpVco1Range, "VCO1 octave range (16' lowest, 2' highest)" },
        { ID::jpPW,        "Pulse width for both VCOs (50% = square)" },
        { ID::jpPWM,       "Pulse-width modulation depth from the PWM LFO" },
        { ID::jpMix,       "Balance: VCO1 (left) to VCO2 (right)" },
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

// ============================================================ label wrap
namespace
{
class Labeled : public juce::Component,
                public juce::SettableTooltipClient
{
public:
    Labeled (juce::Component& c, const juce::String& text) : inner (c)
    {
        addAndMakeVisible (inner);
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (juce::FontOptions (10.f)));
        label.setColour (juce::Label::textColourId, ui::dimText);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }
    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromBottom (13));
        inner.setBounds (b.withSizeKeepingCentre (juce::jmin (b.getWidth() - 6, 110), 24));
    }
private:
    juce::Component& inner;
    juce::Label label;
};
} // namespace

// ============================================================ InsertPanel
namespace
{
// Floating window hosting an external plugin's own editor
class HostedPluginWindow : public juce::DocumentWindow
{
public:
    HostedPluginWindow (juce::AudioPluginInstance& instance,
                        std::function<void (HostedPluginWindow*)> onClose)
        : DocumentWindow (instance.getName(), ui::panel, DocumentWindow::closeButton),
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

InsertPanel::InsertPanel (EightyProcessor& p) : proc (p)
{
    addBtn.setWantsKeyboardFocus (false);
    addBtn.setTooltip ("Load a VST3 effect at the end of the chain (post-FX, pre volume)");
    addBtn.onClick = [this] { showAddMenu(); };
    addAndMakeVisible (addBtn);
    startTimerHz (10);
    refresh();
}

InsertPanel::~InsertPanel()
{
    windows.clear();   // plugin editors must go before the instances do
}

void InsertPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (ui::panel);
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (ui::panelLine);
    g.drawRoundedRectangle (r, 8.f, 1.f);

    g.setColour (ui::accent);
    g.fillRoundedRectangle (10.f, 8.f, 3.f, 12.f, 1.5f);
    g.setColour (ui::dimText);
    g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    g.drawText ("VST3 INSERTS", 19, 6, getWidth() - 24, 15, juce::Justification::centredLeft);

    if (proc.getNumInserts() == 0)
    {
        g.setFont (juce::Font (juce::FontOptions (10.f)));
        g.drawText ("post-FX insert chain", getLocalBounds().reduced (10).withTrimmedTop (20)
                        .withTrimmedBottom (26), juce::Justification::centred);
    }
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
    // close windows whose instance is gone
    for (int w = windows.size(); --w >= 0;)
    {
        auto* hw = static_cast<HostedPluginWindow*> (windows[w]);
        bool alive = false;
        for (int i = 0; i < proc.getNumInserts(); ++i)
            if (proc.getInsert (i) == hw->inst) { alive = true; break; }
        if (! alive)
            windows.remove (w);
    }

    openBtns.clear();
    removeBtns.clear();
    for (int i = 0; i < proc.getNumInserts(); ++i)
    {
        auto* open = openBtns.add (new juce::TextButton (
            juce::String (i + 1) + ". " + proc.getInsertName (i)));
        open->setWantsKeyboardFocus (false);
        open->setTooltip ("Open this plugin's editor");
        open->onClick = [this, i] { openEditorWindow (i); };
        addAndMakeVisible (open);

        auto* rm = removeBtns.add (new juce::TextButton ("x"));
        rm->setWantsKeyboardFocus (false);
        rm->setTooltip ("Remove from the chain");
        rm->onClick = [this, i] { removeInsert (i); };
        addAndMakeVisible (rm);
    }
    addBtn.setEnabled (proc.getNumInserts() < EightyProcessor::kMaxInserts);
    resized();
    repaint();
}

void InsertPanel::resized()
{
    auto b = getLocalBounds().reduced (8);
    b.removeFromTop (18);
    const int rowH = 20;
    for (int i = 0; i < openBtns.size(); ++i)
    {
        auto row = b.removeFromTop (rowH);
        removeBtns[i]->setBounds (row.removeFromRight (rowH).reduced (1));
        openBtns[i]->setBounds (row.reduced (1));
    }
    addBtn.setBounds (b.removeFromBottom (rowH + 2).reduced (1));
}

void InsertPanel::showAddMenu()
{
    juce::PopupMenu m;
    auto types = proc.knownPlugins.getTypes();
    int id = 1;
    for (auto& t : types)
    {
        if (t.isInstrument || t.name == "Eighty") { ++id; continue; }
        m.addItem (id++, t.name + "  (" + t.manufacturerName + ")");
    }
    if (types.isEmpty())
        m.addItem (9001, "Scan VST3 folders...");
    else
    {
        m.addSeparator();
        m.addItem (9001, "Rescan VST3 folders...");
    }

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addBtn),
        [this, types] (int result)
        {
            if (result == 0) return;
            if (result == 9001)
            {
                juce::MouseCursor::showWaitCursor();
                proc.rescanPlugins();
                juce::MouseCursor::hideWaitCursor();
                showAddMenu();
                return;
            }
            const int idx = result - 1;
            if (idx >= 0 && idx < types.size())
            {
                juce::String error;
                if (! proc.addInsert (types.getReference (idx), error))
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::MessageBoxIconType::WarningIcon,
                        "Couldn't load plugin", error);
            }
        });
}

void InsertPanel::openEditorWindow (int index)
{
    auto* inst = proc.getInsert (index);
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

void InsertPanel::removeInsert (int index)
{
    auto* inst = proc.getInsert (index);
    for (int w = windows.size(); --w >= 0;)
        if (static_cast<HostedPluginWindow*> (windows[w])->inst == inst)
            windows.remove (w);        // editor must die before the instance
    proc.removeInsert (index);
}

// ================================================================= Editor
EightyEditor::EightyEditor (EightyProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      scope (p.scopeFifo),
      insertPanel (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    // ---- header
    titleLabel.setText ("EIGHTY", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (22.f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, ui::text);
    addAndMakeVisible (titleLabel);

    chipLabel.setText ("CS-80 ENGINE", juce::dontSendNotification);
    chipLabel.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    chipLabel.setColour (juce::Label::textColourId, ui::accent);
    addAndMakeVisible (chipLabel);

    statusLabel.setFont (juce::Font (juce::FontOptions (11.f)));
    statusLabel.setColour (juce::Label::textColourId, ui::dimText);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel);

    hintLabel.setText ("PLAY: A W S E D F T G Y H U J K O L P ;   |   Z/X octave   C/V velocity   B hold   N arp   |   "
                       "LEFT/RIGHT select control   UP/DOWN adjust   SHIFT+UP/DOWN pitch bend   |   right-click a knob for MIDI learn",
                       juce::dontSendNotification);
    hintLabel.setFont (juce::Font (juce::FontOptions (10.f)));
    hintLabel.setColour (juce::Label::textColourId, ui::dimText);
    hintLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (hintLabel);

    // ---- CS-80: OSC I
    secOsc1.columns = 4;
    makeToggle (secOsc1, ID::osc1On, "ON");
    makeCombo  (secOsc1, ID::osc1Foot, "RANGE");
    makeKnob   (secOsc1, ID::osc1Fine, "FINE");
    makeKnob   (secOsc1, ID::osc1Level, "LEVEL");
    makeKnob   (secOsc1, ID::osc1Saw, "SAW");
    makeKnob   (secOsc1, ID::osc1Pulse, "PULSE");
    makeKnob   (secOsc1, ID::osc1PW, "WIDTH");
    makeKnob   (secOsc1, ID::osc1PWM, "PWM");
    addAndMakeVisible (secOsc1);

    // ---- CS-80: OSC II
    secOsc2.columns = 5;
    makeToggle (secOsc2, ID::osc2On, "ON");
    makeCombo  (secOsc2, ID::osc2Foot, "RANGE");
    makeKnob   (secOsc2, ID::osc2Semi, "SEMI");
    makeKnob   (secOsc2, ID::osc2Fine, "FINE");
    makeKnob   (secOsc2, ID::osc2Level, "LEVEL");
    makeKnob   (secOsc2, ID::osc2Saw, "SAW");
    makeKnob   (secOsc2, ID::osc2Pulse, "PULSE");
    makeKnob   (secOsc2, ID::osc2PW, "WIDTH");
    makeKnob   (secOsc2, ID::osc2PWM, "PWM");
    addAndMakeVisible (secOsc2);

    // ---- CS-80: FILTER
    secFilter.columns = 3;
    makeKnob (secFilter, ID::hpfCutoff, "HP CUT");
    makeKnob (secFilter, ID::lpfCutoff, "LP CUT");
    makeKnob (secFilter, ID::resonance, "RES");
    makeKnob (secFilter, ID::filterEnvAmt, "ENV AMT");
    makeKnob (secFilter, ID::keyTrack, "KEY TRK");
    makeKnob (secFilter, ID::filterDrive, "DRIVE");
    addAndMakeVisible (secFilter);

    // ---- CS-80: ENVELOPES
    secFEnv.columns = 5;
    makeKnob (secFEnv, ID::fEnvA, "ATTACK");
    makeKnob (secFEnv, ID::fEnvD, "DECAY");
    makeKnob (secFEnv, ID::fEnvS, "SUSTAIN");
    makeKnob (secFEnv, ID::fEnvR, "RELEASE");
    makeKnob (secFEnv, ID::velToFilter, "VEL");
    addAndMakeVisible (secFEnv);

    secAEnv.columns = 5;
    makeKnob (secAEnv, ID::aEnvA, "ATTACK");
    makeKnob (secAEnv, ID::aEnvD, "DECAY");
    makeKnob (secAEnv, ID::aEnvS, "SUSTAIN");
    makeKnob (secAEnv, ID::aEnvR, "RELEASE");
    makeKnob (secAEnv, ID::velToAmp, "VEL");
    addAndMakeVisible (secAEnv);

    // ---- JP-8: VCO 1
    secVco1.columns = 3;
    makeCombo (secVco1, ID::jpVco1Wave, "WAVE");
    makeCombo (secVco1, ID::jpVco1Range, "RANGE");
    makeKnob  (secVco1, ID::jpMix, "VCO MIX");
    makeKnob  (secVco1, ID::jpPW, "WIDTH");
    makeKnob  (secVco1, ID::jpPWM, "PWM");
    addAndMakeVisible (secVco1);

    // ---- JP-8: VCO 2
    secVco2.columns = 3;
    makeCombo  (secVco2, ID::jpVco2Wave, "WAVE");
    makeCombo  (secVco2, ID::jpVco2Range, "RANGE");
    makeKnob   (secVco2, ID::jpVco2Semi, "SEMI");
    makeKnob   (secVco2, ID::jpVco2Fine, "FINE");
    makeToggle (secVco2, ID::jpSync, "SYNC");
    makeKnob   (secVco2, ID::jpXmod, "X-MOD");
    addAndMakeVisible (secVco2);

    // ---- JP-8: FILTER
    secJpFilter.columns = 3;
    makeKnob   (secJpFilter, ID::jpHpf, "HP CUT");
    makeKnob   (secJpFilter, ID::jpLpf, "LP CUT");
    makeKnob   (secJpFilter, ID::jpRes, "RES");
    makeToggle (secJpFilter, ID::jpSlope24, "24 dB");
    makeKnob   (secJpFilter, ID::jpEnvAmt, "ENV AMT");
    makeKnob   (secJpFilter, ID::jpKeyTrk, "KEY TRK");
    addAndMakeVisible (secJpFilter);

    // ---- JP-8: ENVELOPES
    secJpFEnv.columns = 5;
    makeKnob (secJpFEnv, ID::jpFEnvA, "ATTACK");
    makeKnob (secJpFEnv, ID::jpFEnvD, "DECAY");
    makeKnob (secJpFEnv, ID::jpFEnvS, "SUSTAIN");
    makeKnob (secJpFEnv, ID::jpFEnvR, "RELEASE");
    makeKnob (secJpFEnv, ID::velToFilter, "VEL");
    addAndMakeVisible (secJpFEnv);

    secJpAEnv.columns = 5;
    makeKnob (secJpAEnv, ID::jpAEnvA, "ATTACK");
    makeKnob (secJpAEnv, ID::jpAEnvD, "DECAY");
    makeKnob (secJpAEnv, ID::jpAEnvS, "SUSTAIN");
    makeKnob (secJpAEnv, ID::jpAEnvR, "RELEASE");
    makeKnob (secJpAEnv, ID::velToAmp, "VEL");
    addAndMakeVisible (secJpAEnv);

    // ---- LFO
    secLfo.columns = 3;
    makeKnob  (secLfo, ID::lfoRate, "RATE");
    makeCombo (secLfo, ID::lfoWave, "WAVE");
    makeKnob  (secLfo, ID::lfoDelay, "DELAY");
    makeKnob  (secLfo, ID::lfoToPitch, "> PITCH");
    makeKnob  (secLfo, ID::lfoToFilter, "> FILTER");
    makeKnob  (secLfo, ID::lfoToAmp, "> AMP");
    addAndMakeVisible (secLfo);

    // ---- TOUCH
    secTouch.columns = 2;
    makeKnob (secTouch, ID::touchRise, "RISE");
    makeKnob (secTouch, ID::touchToVib, "> VIB");
    makeKnob (secTouch, ID::touchToBright, "> BRIGHT");
    makeKnob (secTouch, ID::touchToLevel, "> LEVEL");
    addAndMakeVisible (secTouch);

    // ---- COMMON
    secCommon.columns = 2;
    makeKnob (secCommon, ID::noiseLevel, "NOISE (CS)");
    makeKnob (secCommon, ID::pwmRate, "PWM RATE");
    makeKnob (secCommon, ID::masterTune, "TUNE");
    makeKnob (secCommon, ID::masterVol, "VOLUME");
    addAndMakeVisible (secCommon);

    // ---- ENGINE (view switch + split)
    secEngine.columns = 3;
    viewCsBtn.setClickingTogglesState (true);
    viewJpBtn.setClickingTogglesState (true);
    viewCsBtn.setRadioGroupId (42);
    viewJpBtn.setRadioGroupId (42);
    viewCsBtn.setToggleState (true, juce::dontSendNotification);
    viewCsBtn.setWantsKeyboardFocus (false);
    viewJpBtn.setWantsKeyboardFocus (false);
    viewJpBtn.setColour (juce::TextButton::buttonOnColourId, ui::accent2);
    viewCsBtn.onClick = [this] { setEngineView (false); };
    viewJpBtn.onClick = [this] { setEngineView (true); };
    viewCsBtn.setTooltip ("Show the CS-80 panel. What you hear is set by SOUND");
    viewJpBtn.setTooltip ("Show the JP-8 panel. What you hear is set by SOUND");
    auto* viewWrap = new Labeled (viewCsBtn, "PANEL");
    wrappers.add (viewWrap);
    // place both buttons side by side inside one cell pair
    secEngine.addItem (*viewWrap, 1);
    auto* viewWrap2 = new Labeled (viewJpBtn, "");
    wrappers.add (viewWrap2);
    secEngine.addItem (*viewWrap2, 1);
    makeCombo (secEngine, ID::engineMode, "SOUND");
    {
        auto* k = makeKnob (secEngine, ID::splitPoint, "SPLIT KEY");
        k->slider.textFromValueFunction = [] (double v)
        { return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 3); };
        k->slider.updateText();
    }
    makeToggle (secEngine, ID::splitCsLow, "CS LOW");
    addAndMakeVisible (secEngine);

    // ---- VOICES
    secVoice.columns = 5;
    makeCombo (secVoice, ID::voiceMode, "MODE");
    makeKnob  (secVoice, ID::polyVoices, "VOICES");
    makeKnob  (secVoice, ID::unisonCount, "UNI CNT");
    makeKnob  (secVoice, ID::unisonDetune, "UNI DET");
    makeKnob  (secVoice, ID::stereoSpread, "SPREAD");
    makeKnob  (secVoice, ID::drift, "DRIFT");
    makeKnob  (secVoice, ID::glideTime, "GLIDE");
    makeCombo (secVoice, ID::glideMode, "GLIDE MODE");
    makeKnob  (secVoice, ID::bendRange, "BEND RNG");
    makeToggle(secVoice, ID::hold, "HOLD");
    addAndMakeVisible (secVoice);

    // ---- ARP
    secArp.columns = 4;
    makeToggle (secArp, ID::arpOn, "ON");
    makeCombo  (secArp, ID::arpMode, "MODE");
    makeToggle (secArp, ID::arpSync, "SYNC");
    makeCombo  (secArp, ID::arpDiv, "DIV");
    makeKnob   (secArp, ID::arpRateHz, "RATE HZ");
    makeKnob   (secArp, ID::arpOctaves, "OCTAVES");
    makeKnob   (secArp, ID::arpGate, "GATE");
    addAndMakeVisible (secArp);

    // ---- FX
    secFx.columns = 7;
    makeToggle (secFx, ID::chorusOn, "CHORUS");
    makeKnob   (secFx, ID::chorusRate, "RATE");
    makeKnob   (secFx, ID::chorusDepth, "DEPTH");
    makeKnob   (secFx, ID::chorusMix, "MIX");
    makeToggle (secFx, ID::tremOn, "TREM");
    makeKnob   (secFx, ID::tremRate, "RATE");
    makeKnob   (secFx, ID::tremDepth, "DEPTH");
    makeToggle (secFx, ID::delayOn, "DELAY");
    makeKnob   (secFx, ID::delayTime, "TIME");
    makeToggle (secFx, ID::delaySync, "SYNC");
    makeCombo  (secFx, ID::delayDiv, "DIV");
    makeKnob   (secFx, ID::delayFB, "FEEDBK");
    makeKnob   (secFx, ID::delayMix, "MIX");
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
    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (22.f);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (insertPanel);

    addChildComponent (halo);   // hidden until a control is selected
    halo.setAlwaysOnTop (true);

    setEngineView (false);
    setWantsKeyboardFocus (true);
    addKeyListener (this);
    startTimerHz (30);
    setSize (1340, 736);
}

EightyEditor::~EightyEditor()
{
    removeKeyListener (this);
    setLookAndFeel (nullptr);
}

// -------------------------------------------------------- control makers
Knob* EightyEditor::makeKnob (Section& s, const juce::String& paramID,
                              const juce::String& label, int span)
{
    auto* k = new Knob (label);
    knobs.add (k);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        proc.apvts, paramID, k->slider));
    if (auto* param = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true,
            param->convertFrom0to1 (param->getDefaultValue()));
    k->slider.onRightClick = [this, paramID] (juce::Point<int> pos)
    { showLearnMenu (paramID, pos); };
    const auto tip = tipFor (paramID);
    k->setTooltip (tip);
    k->slider.setTooltip (tip);
    s.addItem (*k, span);
    controls.push_back ({ k, paramID });
    k->addMouseListener (this, true);
    return k;
}

juce::ComboBox* EightyEditor::makeCombo (Section& s, const juce::String& paramID,
                                         const juce::String& label, int span)
{
    auto* c = new juce::ComboBox();
    combos.add (c);
    c->setWantsKeyboardFocus (false);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramID)))
        c->addItemList (param->choices, 1);
    comboAtts.add (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (
        proc.apvts, paramID, *c));

    auto* wrap = new Labeled (*c, label);
    wrappers.add (wrap);
    const auto tip = tipFor (paramID);
    wrap->setTooltip (tip);
    c->setTooltip (tip);
    s.addItem (*wrap, span);
    controls.push_back ({ wrap, paramID });
    wrap->addMouseListener (this, true);
    return c;
}

juce::ToggleButton* EightyEditor::makeToggle (Section& s, const juce::String& paramID,
                                              const juce::String& label, int span)
{
    auto* t = new juce::ToggleButton (label);
    toggles.add (t);
    t->setWantsKeyboardFocus (false);
    buttonAtts.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (
        proc.apvts, paramID, *t));
    t->setTooltip (tipFor (paramID));
    s.addItem (*t, span);
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

    viewCsBtn.setToggleState (! jupiter, juce::dontSendNotification);
    viewJpBtn.setToggleState (jupiter, juce::dontSendNotification);
    chipLabel.setText (jupiter ? "JP-8 ENGINE" : "CS-80 ENGINE", juce::dontSendNotification);
    chipLabel.setColour (juce::Label::textColourId, jupiter ? ui::accent2 : ui::accent);

    // drop selection if the selected control just got hidden
    if (selectedCtl >= 0 && ! controls[(size_t) selectedCtl].target->isShowing())
        selectControl (-1);
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
    auto* p = proc.apvts.getParameter (controls[(size_t) selectedCtl].paramID);
    if (p == nullptr) return;

    const int steps = p->getNumSteps();
    const float delta = (steps > 1 && steps <= 128) ? 1.f / (float) (steps - 1) : 0.02f;
    const float nv = juce::jlimit (0.f, 1.f, p->getValue() + (float) dir * delta);
    p->beginChangeGesture();
    p->setValueNotifyingHost (nv);
    p->endChangeGesture();
}

void EightyEditor::mouseDown (const juce::MouseEvent& e)
{
    // click-to-select: map the clicked component back to its control
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
    g.fillAll (ui::bg);
    g.setColour (ui::panelLine);
    g.drawHorizontalLine (44, 10.f, (float) getWidth() - 10.f);

    auto chip = chipLabel.getBounds().expanded (6, 2).toFloat();
    g.setColour ((jpView ? ui::accent2 : ui::accent).withAlpha (0.55f));
    g.drawRoundedRectangle (chip, 9.f, 1.f);
}

void EightyEditor::resized()
{
    const int m = 12, gap = 8;
    auto full = getLocalBounds().reduced (m, 0);

    titleLabel.setBounds (m + 2, 8, 110, 28);
    chipLabel.setBounds (m + 122, 15, 86, 15);
    statusLabel.setBounds (getWidth() - 460, 12, 444, 22);

    int y = 50;
    const int rowH = 172;

    // row A (CS + JP panels share the same rects)
    {
        int x = full.getX();
        secOsc1.setBounds (x, y, 300, rowH);
        secVco1.setBounds (x, y, 300, rowH);          x += 300 + gap;
        secOsc2.setBounds (x, y, 350, rowH);
        secVco2.setBounds (x, y, 350, rowH);          x += 350 + gap;
        secFilter.setBounds (x, y, 264, rowH);
        secJpFilter.setBounds (x, y, 264, rowH);      x += 264 + gap;
        scope.setBounds (x, y, full.getRight() - x, rowH);
    }
    y += rowH + gap;

    // row B
    {
        int x = full.getX();
        secFEnv.setBounds (x, y, 280, rowH);
        secJpFEnv.setBounds (x, y, 280, rowH);        x += 280 + gap;
        secAEnv.setBounds (x, y, 280, rowH);
        secJpAEnv.setBounds (x, y, 280, rowH);        x += 280 + gap;
        secLfo.setBounds (x, y, 250, rowH);           x += 250 + gap;
        secTouch.setBounds (x, y, 190, rowH);         x += 190 + gap;
        secCommon.setBounds (x, y, full.getRight() - x, rowH);
    }
    y += rowH + gap;

    // row C
    {
        int x = full.getX();
        secEngine.setBounds (x, y, 230, rowH);        x += 230 + gap;
        secVoice.setBounds (x, y, 400, rowH);         x += 400 + gap;
        secArp.setBounds (x, y, 285, rowH);           x += 285 + gap;
        secFx.setBounds (x, y, full.getRight() - x, rowH);
    }
    y += rowH + gap;

    const int kbH = getHeight() - y - 22;
    const int insertW = 250;
    wheel.setBounds (full.getX(), y, 46, kbH);
    keyboard.setBounds (full.getX() + 46 + gap, y,
                        full.getWidth() - 46 - insertW - gap * 2, kbH);
    insertPanel.setBounds (full.getRight() - insertW, y, insertW, kbH);
    hintLabel.setBounds (0, getHeight() - 18, getWidth(), 14);

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
            return true;                        // pitch bend, handled in timer
        adjustSelected (code == juce::KeyPress::upKey ? 1 : -1);
        return true;
    }
    if (code == juce::KeyPress::escapeKey) { selectControl (-1); return true; }

    const auto c = (juce::juce_wchar) juce::CharacterFunctions::toLowerCase (
        (juce::juce_wchar) kp.getTextCharacter());
    if (juce::String (noteKeys).containsChar (c)) return true;   // handled in state scan
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

    // auto-switch panel when the SOUND engine selection changes
    // (Split and Layer use both engines - keep whichever panel is up)
    const int mode = (int) proc.apvts.getRawParameterValue (ID::engineMode)->load();
    if (mode != lastEngineMode)
    {
        if (lastEngineMode >= 0 && mode != 2 && mode != 3)
            setEngineView (mode == 1);
        lastEngineMode = mode;
    }

    // SHIFT + up/down arrows: pitch bend
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

    // status line
    juce::String status;
    if (proc.midiLearn.isLearning())
    {
        statusLabel.setColour (juce::Label::textColourId, ui::learn);
        status = "MIDI LEARN: move a control on your MIDI device...";
    }
    else
    {
        statusLabel.setColour (juce::Label::textColourId, ui::dimText);
        if (proc.lastLearnedCC.load() >= 0)
            learnFlashCC = proc.lastLearnedCC.load();
        status = "VOICES " + juce::String (proc.activeVoices.load()) + "/16"
               + "   OCT C" + juce::String (baseNote / 12 - 1)
               + "   VEL " + juce::String (typeVelocity, 1);
        if (selectedCtl >= 0 && selectedCtl < (int) controls.size())
            if (auto* p = proc.apvts.getParameter (controls[(size_t) selectedCtl].paramID))
                status += "   [" + p->getName (24) + ": " + p->getCurrentValueAsText() + "]";
        if (learnFlashCC >= 0)
            status += "   [last mapped: CC " + juce::String (learnFlashCC) + "]";
    }
    statusLabel.setText (status, juce::dontSendNotification);
}
