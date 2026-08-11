#pragma once
#include "PluginProcessor.h"

// ---------------------------------------- palette: "Refined Cream Strip"
namespace ui
{
    const juce::Colour winBg     { 0xffece6da };   // cream panel
    const juce::Colour cardBg    { 0xfff7f3ea };   // lighter cream (boxes)
    const juce::Colour line      { 0xffc9c0af };   // hairline dividers
    const juce::Colour track     { 0xffb8ae9a };   // fader tracks / off borders
    const juce::Colour ink       { 0xff2a2723 };   // primary dark
    const juce::Colour inkSoft   { 0xff4a443b };   // fader labels
    const juce::Colour mid       { 0xff6a6357 };   // knob labels
    const juce::Colour dim       { 0xff8d8578 };   // secondary text
    const juce::Colour cream     { 0xfff4efe6 };   // text on ink
    const juce::Colour ledOn     { 0xffb6412f };   // LED / selection
    const juce::Colour ledOff    { 0x2e2a2723 };
    const juce::Colour scopeBg   { 0xff201d18 };
    const juce::Colour scopeLine { 0xff3a352c };
    const juce::Colour scopeTrace{ 0xffe8a33d };   // amber: traces + value arcs
    const juce::Colour jpAccent  { 0xff5f57a8 };   // Jupiter-8 identity
    const juce::Colour wheelBg   { 0xffddd5c4 };
    const juce::Colour keyDown   { 0xffeec18a };

    // section stripes
    const juce::Colour stLfo   { 0xffb6412f };
    const juce::Colour stOsc   { 0xffc97f2e };     // doubles as the CS accent
    const juce::Colour stMix   { 0xffa08a2f };
    const juce::Colour stFilt  { 0xff3f6ea5 };
    const juce::Colour stEnv   { 0xff4a8a58 };
    const juce::Colour stTouch { 0xff7d5a9e };
    const juce::Colour stVoice { 0xff3d8f83 };

    juce::Font sans (float size, bool bold = false);
    juce::Font mono (float size);

    // Fixed band layout, top to bottom. The window height is the sum of
    // these, so a band only has to be edited in one place.
    constexpr int headerH = 56;
    constexpr int tabY    = headerH,      tabH    = 32;
    constexpr int engineY = tabY + tabH,  engineH = 180;   // 88
    // Performance band: the two CS-80 macro sliders, channel II, the ring
    // modulator, render quality and the effects. Shorter than the other
    // bands because everything in it is a knob or a chip stack.
    constexpr int perfY   = engineY + engineH, perfH   = 120;   // 268
    constexpr int sharedY = perfY + perfH, sharedH = 180;       // 388
    // Sequencer band: transport + tempo, the two track strips, the step grid,
    // and under the grid a readout strip naming the pattern's scale and the
    // tempo actually driving the step clock.
    constexpr int seqY    = sharedY + sharedH, seqH    = 124;   // 568
    constexpr int footerY = seqY + seqH,  footerH = 126;   // 692
    constexpr int hintY   = footerY + footerH + 2;         // 820
    constexpr int windowW = 1720, windowH = hintY + 20;    // 840
}

// ------------------------------------------------------------ LookAndFeel
class CreamLNF : public juce::LookAndFeel_V4
{
public:
    CreamLNF();
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getPopupMenuFont() override;
};

// -------------------------------------------------- slider w/ MIDI learn
class LearnSlider : public juce::Slider
{
public:
    std::function<void (juce::Point<int>)> onRightClick;
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onRightClick)
            onRightClick (e.getScreenPosition());
        else
            juce::Slider::mouseDown (e);
    }
};

// ---------------------------------------------------------- fader / knob
// Both carry a persistent mono value readout under the name label.
class VFader : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit VFader (const juce::String& labelText);
    void resized() override;
    LearnSlider slider;
    juce::Label label, value;
};

class MiniKnob : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MiniKnob (const juce::String& labelText, bool headerStyle = false);
    void resized() override;
    LearnSlider slider;
    juce::Label label, value;
    int knobSize = 0;      // 0 = derive from width (slot - 6)
private:
    bool header;
};

// -------------------------------------------------------------- chip stack
// Vertical (or horizontal) chip selector bound to a choice parameter.
class ChipStack : public juce::Component, public juce::SettableTooltipClient
{
public:
    ChipStack (juce::RangedAudioParameter& param, juce::StringArray labels,
               const juce::String& groupLabel, bool horizontal = false,
               juce::Colour onColour = ui::ink);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void()> onUserChange;
    std::function<void (juce::Point<int>)> onRightClick;
    juce::String paramID;
    int cellW = 55;          // horizontal mode only

private:
    int chipH() const { return labels.size() >= 6 ? 15 : 17; }
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    juce::StringArray labels;
    juce::String group;
    bool horiz;
    juce::Colour onCol;
    int selected = 0;
};

// -------------------------------------------------------------- LED toggle
class LedToggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    LedToggle (juce::RangedAudioParameter& param, const juce::String& text);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void()> onUserChange;
    std::function<void (juce::Point<int>)> onRightClick;
    juce::String paramID;
    int preferredWidth() const;

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    juce::String text;
    bool on = false;
};

// ----------------------------------------------------------- mini display
// Small amber-on-dark readout at a section's top-right: filter response,
// ADSR shape or LFO waveform, redrawn from the current parameter values.
class MiniDisplay : public juce::Component, private juce::Timer
{
public:
    enum Kind { filterKind, adsrKind, lfoKind };
    MiniDisplay (Kind, std::vector<juce::RangedAudioParameter*> params);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    Kind kind;
    std::vector<juce::RangedAudioParameter*> params;
    std::vector<float> cache;
};

// ----------------------------------------------------------------- section
// Flat strip section: hairline left border, name + colored underline,
// LED toggles (or a mini display) top-right, content items below.
class Section : public juce::Component
{
public:
    Section (const juce::String& name, juce::Colour stripe);
    void paint (juce::Graphics&) override;
    void resized() override;
    void addItem (juce::Component& c, int width);
    void addHeaderToggle (LedToggle& t);
    void setDisplay (MiniDisplay* d, int width);
    int preferredWidth() const;
    void setStripe (juce::Colour c) { stripe = c; repaint(); }

private:
    juce::String name;
    juce::Colour stripe;
    struct Item { juce::Component* comp; int width; };
    std::vector<Item> items;
    std::vector<LedToggle*> headerToggles;
    MiniDisplay* display = nullptr;
    int displayW = 88;
};

// ------------------------------------------------------------------ scope
// One tap drains the stereo scope FIFO and keeps the ring buffers; the
// waveform trace and the lissajous both read from it, since a FIFO can
// only be consumed once.
class ScopeTap : private juce::Timer
{
public:
    static constexpr int kHistory = 8192, kMask = kHistory - 1;

    explicit ScopeTap (ScopeFifo& fifo);
    void addView (juce::Component* c) { views.push_back (c); }

    float left (int framesBack) const  { return l[(size_t) ((writePos - framesBack) & kMask)]; }
    float right (int framesBack) const { return r[(size_t) ((writePos - framesBack) & kMask)]; }
    int   pos() const { return writePos; }
    float at (const std::vector<float>& buf, int idx) const { return buf[(size_t) (idx & kMask)]; }
    const std::vector<float>& lBuf() const { return l; }
    const std::vector<float>& rBuf() const { return r; }

private:
    void timerCallback() override;
    ScopeFifo& fifo;
    std::vector<float> l, r;
    std::vector<juce::Component*> views;
    int writePos = 0;
};

class ScopeComponent : public juce::Component
{
public:
    explicit ScopeComponent (ScopeTap& tap);
    void paint (juce::Graphics&) override;

private:
    ScopeTap& tap;
};

// Vector display: L/R plotted rotated 45 degrees, so a mono signal draws a
// vertical line and stereo width opens it out sideways.
class LissajousScope : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit LissajousScope (ScopeTap& tap);
    void paint (juce::Graphics&) override;

private:
    ScopeTap& tap;
};

// ------------------------------------------------------------ pitch wheel
class PitchWheel : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void (float, bool)> onChange;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setExternalValue (float v);

private:
    void setFromMouse (const juce::MouseEvent&);
    float value = 0.f;
};

// --------------------------------------------------- engine panel tabs
class PanelChips : public juce::Component
{
public:
    std::function<void (bool)> onSelect;   // true = JP panel
    void setJp (bool jp) { jpSel = jp; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    static constexpr int tabW = 180, tabGap = 6, totalW = tabW * 2 + tabGap;

private:
    bool jpSel = false;
};

// ----------------------------------------------------- key zone strip
// Sits above the keyboard. In Split mode it shows the two zones and a
// click moves the split point; in Keys mode each key is painted with the
// engine it triggers (click cycles CS -> JP -> BOTH, drag paints,
// right-click opens a fill menu).
class KeyZoneStrip : public juce::Component, public juce::SettableTooltipClient
{
public:
    KeyZoneStrip (EightyProcessor&, juce::MidiKeyboardComponent&);
    void setMode (int engineMode);           // 2 = split, 4 = keys
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    int noteAt (juce::Point<float>) const;
    void applyPaint (int note);
    void showFillMenu();

    EightyProcessor& proc;
    juce::MidiKeyboardComponent& kb;
    int mode = 2;
    uint8_t paintValue = 0;
};

// -------------------------------------------------------- sequencer grid
// Two tracks x 16 steps, drawn as a ruler plus two rows of cells. Click
// moves the edit cursor (and drops any held notes into the step), vertical
// drag transposes a step, right-click opens the step/track menu. The
// playhead and the cursor are both drawn live.
class SeqGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SeqGrid (EightyProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    std::function<void (const juce::String&)> onMessage;   // -> LCD readout

    static constexpr int rulerH = 14, rowH = 32, rowGap = 2;
    static int preferredHeight() { return rulerH + rowH * 2 + rowGap; }

private:
    int trackAt (juce::Point<int>) const;
    int stepAt (juce::Point<int>) const;
    juce::Rectangle<int> cellBounds (int track, int step) const;
    void setCursor (int track, int step);
    void showStepMenu (int track, int step);
    void say (const juce::String& s) { if (onMessage) onMessage (s); }

    EightyProcessor& proc;
    int dragTrack = -1, dragStep = -1, dragApplied = 0;
    int dragStartX = 0, dragStartY = 0;
    int dragAxis = 0;            // 0 undecided, 1 transpose, 2 sustain length
    int dragBaseHold = 1;
};

// -------------------------------------------- selection ring (arrow keys)
class SelectionHalo : public juce::Component
{
public:
    SelectionHalo() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (ui::ledOn.withAlpha (0.85f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 5.f, 2.f);
    }
};

// ---------------------------------------------------- plugin chooser
// Type-to-search list of scanned plugins, shown in a call-out from the
// "+ FX" / "+ SET SYNTH" buttons. Typing filters on name and manufacturer,
// up/down moves, Return loads, Escape closes.
class PluginChooser : public juce::Component,
                      private juce::ListBoxModel,
                      private juce::TextEditor::Listener,
                      private juce::KeyListener
{
public:
    PluginChooser (juce::Array<juce::PluginDescription> items,
                   const juce::String& title,
                   std::function<void (const juce::PluginDescription&)> onPick,
                   std::function<void()> onRescan);

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    int getNumRows() override { return filtered.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void returnKeyPressed (int row) override;
    void textEditorTextChanged (juce::TextEditor&) override;
    void textEditorReturnKeyPressed (juce::TextEditor&) override;
    void textEditorEscapeKeyPressed (juce::TextEditor&) override;
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;

    void refilter();
    void choose (int row);
    void dismiss();

    juce::Array<juce::PluginDescription> all;
    juce::Array<int> filtered;
    juce::String title;
    std::function<void (const juce::PluginDescription&)> pick;
    std::function<void()> rescan;

    juce::TextEditor search;
    juce::ListBox list { {}, this };
    juce::TextButton rescanBtn { "RESCAN VST3 FOLDERS" };
};

// ------------------------------------------- external VST3 insert panel
class InsertPanel : public juce::Component, private juce::Timer
{
public:
    InsertPanel (EightyProcessor&, std::function<void (const juce::String&)> touched);
    ~InsertPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refresh();
    void showChooser (bool instruments);
    void openWindowFor (juce::AudioPluginInstance*);
    void closeWindowFor (juce::AudioPluginInstance*);

    EightyProcessor& proc;
    juce::TextButton addBtn { "+ FX" }, synthBtn { "+ SET SYNTH" },
                     synthClearBtn { "x" };
    juce::OwnedArray<juce::TextButton> openBtns, removeBtns;
    juce::OwnedArray<juce::DocumentWindow> windows;
    int lastVersion = -1;
};

// ------------------------------------------------------------------ editor
class EightyEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     private juce::KeyListener
{
public:
    explicit EightyEditor (EightyProcessor&);
    ~EightyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;
    bool keyStateChanged (bool isKeyDown, juce::Component*) override;
    void mouseDown (const juce::MouseEvent&) override;

    void paramTouched (const juce::String& paramID);

private:
    void timerCallback() override;
    void scanNoteKeys();
    void handleActionKey (juce::juce_wchar c);
    void showLearnMenu (const juce::String& paramID, juce::Point<int> screenPos);
    void setEngineView (bool jupiter);
    void updateModeVisibility (int mode);
    void layoutRows();

    struct Ctl { juce::Component* target; juce::String paramID; };
    void selectControl (int index);
    void moveSelection (int dir);
    void adjustSelected (float amount);
    void tickAdjustRamp();
    void updateHalo();

    VFader*   makeFader (Section&, const juce::String& paramID, const juce::String& label,
                         int width = 28);
    MiniKnob* makeKnob  (Section&, const juce::String& paramID, const juce::String& label,
                         int width = 40);
    ChipStack* makeChips (Section&, const juce::String& paramID,
                          juce::StringArray labels, const juce::String& group, int width = 48);
    LedToggle* makeLed  (Section&, const juce::String& paramID, const juce::String& label);
    MiniKnob* makeHeaderKnob (const juce::String& paramID, const juce::String& label);
    MiniDisplay* makeDisplay (Section&, MiniDisplay::Kind,
                              std::initializer_list<const char*> paramIDs, int width);
    void wireSlider (LearnSlider&, juce::Label& value, const juce::String& paramID);

    // preset bar (tab band)
    void buildPresetBar();
    void showPresetMenu();
    void promptSavePreset();
    void stepPreset (int dir);
    void applyPreset (const juce::File&);
    void refreshPresetName();

    // sequencer row / mixer: controls that live loose on the editor, not in
    // a Section
    LedToggle*  makeLooseLed (const juce::String& paramID, const juce::String& label);
    ChipStack*  makeLooseChips (const juce::String& paramID, juce::StringArray labels,
                                const juce::String& group, int cellW);
    VFader*     makeLooseFader (const juce::String& paramID, const juce::String& label);
    MiniKnob*   makeLooseKnob (const juce::String& paramID, const juce::String& label);
    void buildSeqRow();
    void buildMixer();
    void updateSeqReadout();

    EightyProcessor& proc;
    CreamLNF lnf;

    // shared sections (row 2)
    Section secLfo     { "LFO",       ui::stLfo },
            secTouch   { "TOUCH",     ui::stTouch },
            secVoiceCS { "CS VOICES", ui::stVoice },
            secVoiceJP { "JP VOICES", ui::stVoice },
            secGlide   { "GLOBAL",    ui::stVoice },
            secArp     { "ARP",       ui::stLfo };
    // performance row
    Section secCh2     { "CH II",     ui::stOsc },
            secRing    { "RING MOD",  ui::stMix },
            secPerform { "PERFORM",   ui::stTouch },
            secQuality { "QUALITY",   ui::stVoice },
            secFx      { "EFFECTS",   ui::stFilt };
    // CS-80 sections (engine row)
    Section secOsc1  { "OSC I",      ui::stOsc },
            secOsc2  { "OSC II",     ui::stOsc },
            secMix   { "MIX",        ui::stMix },
            secFilter{ "FILTER",     ui::stFilt },
            secFEnv  { "FILTER ENV", ui::stEnv },
            secAEnv  { "AMP ENV",    ui::stEnv };
    // Jupiter-8 sections (engine row, unified JP accent stripe)
    Section secVco1    { "VCO 1",        ui::jpAccent },
            secVco2    { "VCO 2",        ui::jpAccent },
            secJpMix   { "MIX",          ui::jpAccent },
            secJpFilter{ "FILTER 12/24", ui::jpAccent },
            secJpFEnv  { "F.ENV",        ui::jpAccent },
            secJpAEnv  { "A.ENV",        ui::jpAccent };

    ScopeTap scopeTap;
    ScopeComponent scope;
    LissajousScope lissajous;
    PitchWheel wheel;
    InsertPanel insertPanel;

    // sequencer row
    SeqGrid seqGrid;
    LedToggle *seqRecLed = nullptr, *seqPlayLed = nullptr,
              *seqMuteLed[2] = { nullptr, nullptr };
    ChipStack *seqTrackChips = nullptr, *seqEngChips[2] = { nullptr, nullptr };
    juce::TextButton seqClearBtn { "CLEAR" }, seqCopyBtn { "COPY" };
    MiniKnob* tempoKnob = nullptr;
    juce::Label seqScaleLabel, seqTempoLabel;   // strip under the grid
    // panel backgrounds painted behind loose controls, set in resized()
    juce::Rectangle<int> seqStripArea, mixerArea;

    // output mixer (footer, beside the plugin loader): CS-80 / JP-8 / VST3
    // synth layer, each with a level fader, mute and solo
    static constexpr int kMixStrips = 3;
    VFader*    mixFader[kMixStrips] = {};
    LedToggle* mixMute[kMixStrips]  = {};
    LedToggle* mixSolo[kMixStrips]  = {};

    // preset bar
    juce::TextButton presetPrevBtn { "<" }, presetNextBtn { ">" },
                     presetNameBtn { "Init" }, presetSaveBtn { "SAVE" };
    std::unique_ptr<juce::AlertWindow> saveWindow;
    juce::String shownPresetName;
    juce::MidiKeyboardComponent keyboard;
    juce::Label titleLabel, subLabel, statusLabel, hintLabel;
    PanelChips panelChips;
    std::unique_ptr<ChipStack> engineChips;
    MiniKnob *splitKnob = nullptr, *balKnob = nullptr, *volKnob = nullptr,
             *tuneKnob = nullptr, *limitKnob = nullptr, *widthKnob = nullptr;
    std::unique_ptr<KeyZoneStrip> zoneStrip;
    std::unique_ptr<LedToggle> csLowLed;
    SelectionHalo halo;
    juce::TooltipWindow tooltipWindow { this, 400 };
    bool jpView = false;

    juce::OwnedArray<juce::Component> owned;   // faders, knobs, chips, leds, displays
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtts;

    std::vector<Ctl> controls;
    int selectedCtl = -1;
    int lastEngineMode = -1;

    // arrow-key value ramp: holding up/down sweeps continuously instead of
    // firing one discrete step per key repeat
    int   adjustDir = 0;
    float adjustHeld = 0.f;     // seconds the key has been down (accelerates)
    float adjustCarry = 0.f;    // sub-step remainder for stepped parameters
    juce::RangedAudioParameter* adjustParam = nullptr;   // open gesture, if any

    // LCD readout (tab band)
    juce::String readoutText;
    juce::uint32 readoutUntil = 0;

    // computer-keyboard playing state
    int baseNote = 48;
    float typeVelocity = 0.8f;
    bool noteKeyDown[32] {};
    float keyBend = 0.f;
    int bendDir = 0;
    bool wheelActive = false;
    int learnFlashCC = -1;

    static constexpr const char* noteKeys = "awsedftgyhujkolp;";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EightyEditor)
};
