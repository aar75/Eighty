#pragma once
#include "PluginProcessor.h"
#include "Gesture.h"

// ------------------------------------------------- palette: "Eighty Dark"
// Dark blue-grey, from the Claude Design handoff. The names carry the same
// roles they did in the old cream theme - `ink` is still "strongest
// foreground", `dim` still "secondary text" - so paint code did not have to
// be re-reasoned, only re-pointed. Anything that is a *surface* rather than
// a colour role got its own name below.
namespace ui
{
    const juce::Colour winBg     { 0xff232b3b };   // page background
    const juce::Colour bandBg    { 0xff2b3446 };   // header / tab / seq / footer bands
    const juce::Colour cardBg    { 0xff323d52 };   // section cards
    const juce::Colour line      { 0xff4c5b7c };   // borders / dividers
    const juce::Colour track     { 0xff4f5f82 };   // inactive borders
    const juce::Colour ink       { 0xffe8edf7 };   // primary foreground
    const juce::Colour inkSoft   { 0xffccd6e6 };   // fader labels
    const juce::Colour mid       { 0xffccd6e6 };   // knob labels
    const juce::Colour dim       { 0xff8c9ab5 };   // secondary text
    const juce::Colour ledOn     { 0xffff7a52 };   // LED / selection
    const juce::Colour ledOff    { 0xff5f6c84 };
    const juce::Colour scopeBg   { 0xff0f141c };   // scopes, LCDs, value readouts
    const juce::Colour scopeLine { 0xff2c3850 };
    const juce::Colour scopeTrace{ 0xffffb454 };   // amber: traces + value arcs
    const juce::Colour jpAccent  { 0xff8f86e8 };   // Jupiter-8 identity
    const juce::Colour wheelBg   { 0xff2f3a4f };
    const juce::Colour keyDown   { 0xffffb454 };

    // control chrome
    const juce::Colour ctrlBg    { 0xff3a4660 };   // buttons, LED pills
    const juce::Colour ctrlLine  { 0xff546488 };
    const juce::Colour chipOff   { 0xff36415a };
    const juce::Colour knobFace  { 0xff3e4a63 };
    const juce::Colour knobRim   { 0xff5e6d8c };
    const juce::Colour knobHub   { 0xff4e5b74 };
    const juce::Colour groove    { 0xff232c3e };   // fader track
    const juce::Colour thumbHi   { 0xff71809c };   // fader cap gradient
    const juce::Colour thumbLo   { 0xff5e6d8c };
    const juce::Colour thumbEdge { 0xff8b9ab8 };
    const juce::Colour textHi    { 0xfff2f5fb };   // titles / selected chip text
    const juce::Colour dimmer    { 0xff7484a0 };   // placeholder text

    // section stripes
    const juce::Colour stLfo   { 0xffe06552 };
    const juce::Colour stOsc   { 0xffe8a33d };     // doubles as the CS accent
    const juce::Colour stMix   { 0xffc9b458 };
    const juce::Colour stFilt  { 0xff5b9dd9 };
    const juce::Colour stEnv   { 0xff5fb878 };
    const juce::Colour stTouch { 0xffa583d9 };
    const juce::Colour stVoice { 0xff45b3a0 };

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
class EightyLNF : public juce::LookAndFeel_V4
{
public:
    EightyLNF();
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

// ------------------------------------------------ button w/ right-click
// In the LearnSlider idiom: a plain button that also offers a context menu.
class MenuButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;
    std::function<void()> onRightClick;
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onRightClick) onRightClick();
        else juce::TextButton::mouseDown (e);
    }
};

// ---------------------------------------------------------- fader / knob
// Both carry a persistent mono value readout under the name label.
// The readout sits in an inset dark chip, so amber-on-dark reads the same
// everywhere: value boxes, LCD, scopes.
class VFader : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit VFader (const juce::String& labelText);
    void paint (juce::Graphics&) override;
    void resized() override;
    LearnSlider slider;
    juce::Label label, value;
};

class MiniKnob : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MiniKnob (const juce::String& labelText, bool headerStyle = false);
    void paint (juce::Graphics&) override;
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
               juce::Colour onColour = ui::scopeTrace);
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
//
// These are plotted from the parameters' *real* units (Hz, seconds), not
// from the normalised 0-1 positions, so the picture tracks what the DSP
// actually does: the filter trace is a magnitude response on a log
// frequency axis with the resonance peaks where the filter puts them, and
// the envelope's segment widths are its real times, root-compressed so a
// 10-second release still leaves room for the attack.
//
// Parameter order per kind:
//   filterKind - hpf, lpf, res [, hpfRes]
//   adsrKind   - a, d, s, r [, initialLevel, attackLevel]
//   lfoKind    - wave
class MiniDisplay : public juce::Component,
                    public juce::SettableTooltipClient,
                    private juce::Timer
{
public:
    enum Kind { filterKind, adsrKind, lfoKind };
    MiniDisplay (Kind, std::vector<juce::RangedAudioParameter*> params);
    void paint (juce::Graphics&) override;

    juce::Colour trace = ui::scopeTrace;

private:
    void timerCallback() override;
    float value (size_t i, float fallback = 0.f) const;   // real units
    static float filterMagDb (float f, float lpHz, float hpHz, float q, float qHp);

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
    juce::Colour stripeColour() const { return stripe; }   // controls' accent

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

// Triggered waveform trace over a divided graticule. The trace is drawn
// three times - a wide, faint pass, a medium one, then the solid line - so
// it blooms like a phosphor scope instead of reading as a hairline, and a
// translucent fill under it gives the trace some body at low levels.
class ScopeComponent : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ScopeComponent (ScopeTap& tap);
    void paint (juce::Graphics&) override;

private:
    ScopeTap& tap;
};

// Vector display: L/R plotted rotated 45 degrees, so a mono signal draws a
// vertical line and stereo width opens it out sideways.
//
// Drawn with phosphor persistence: each frame fades the previous one
// towards the background instead of clearing it, so the shape a patch
// traces over time stays visible rather than flickering one frame at a
// time. The decay lives in an offscreen image because JUCE hands paint()
// a cleared surface every frame.
class LissajousScope : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit LissajousScope (ScopeTap& tap);
    void paint (juce::Graphics&) override;

private:
    ScopeTap& tap;
    juce::Image phosphor;
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

// ------------------------------------------------------- touch monitor
// Draws the trackpad as the surface it is being played as: where each finger
// is, which string it is sitting on, which engine it drives and what it last
// struck. Off by default (SETTINGS > Show touch monitor), and even then only
// on screen while a gesture key is held - there is nothing to watch
// otherwise, and it sits over the panel while it is up.
//
// Everything here is read back from the gesture engine and the synth rather
// than recomputed, so what you see is what was actually played: the string
// grid comes from GestureEngine::slots, the notes from the same latched set
// the strum lays out, and the ringing dots from the voices still sounding.
class TouchMonitor : public juce::Component, private juce::Timer
{
public:
    TouchMonitor (EightyProcessor&, const eighty::GestureEngine&);
    void paint (juce::Graphics&) override;
    void setActive (bool);

    static constexpr int prefW = 566, prefH = 244;

private:
    void timerCallback() override { repaint(); }

    // The pad is drawn to a real trackpad's proportions, and the strum axis
    // may be either of them - so positions go through one mapping rather
    // than being written out twice.
    juce::Point<float> padPoint (juce::Rectangle<float> pad, float along, float cross) const;
    void paintPad (juce::Graphics&, juce::Rectangle<float> pad,
                   const int* notes, int numNotes, int slots);
    void paintLane (juce::Graphics&, juce::Rectangle<int> area, int lane,
                    const int* notes, int numNotes, int slots);
    static juce::Colour modelColour (int model);

    EightyProcessor& proc;
    const eighty::GestureEngine& gesture;
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
    void showSettingsMenu();

    // ---- trackpad gestures -------------------------------------------
    // The pad is captured only while a gesture key is held, so scanning the
    // keys is what arms and disarms the whole thing. Touches come back on
    // the message thread and leave through the processor's gesture queue -
    // the editor never speaks to the engine directly.
    void setupTrackpad();
    void scanGestureKeys();
    void applyGestureSettings();
    void setGestureMode (eighty::GestureEngine::Mode);
    float gestureParam (const char* id) const;
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

    // preset bar (tab band). The list the < > buttons walk and the menu shows
    // is the built-in starting points followed by the user's saved files, so
    // there is one ordering and one notion of "the next preset".
    struct PresetEntry
    {
        int factoryIndex = -1;      // >= 0 for a factory starting point
        juce::File file;            // otherwise a saved .eighty file
        juce::String name;
    };
    std::vector<PresetEntry> presetEntries() const;
    void applyPresetEntry (const PresetEntry&);

    // audio recorder (header): one button, a running time, and the file it
    // wrote. Not a parameter - see EightyProcessor::startRecording.
    void buildRecorder();
    void toggleRecording();
    void showRecorderMenu();
    void updateRecorder();

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
    EightyLNF lnf;

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
                     presetNameBtn { "Init" }, presetSaveBtn { "SAVE" },
                     settingsBtn { "SETTINGS" };
    MenuButton recBtn { "RECORD" };
    juce::Label recTime;
    std::unique_ptr<juce::AlertWindow> saveWindow;
    juce::String shownPresetName;
    juce::MidiKeyboardComponent keyboard;
    juce::Label titleLabel, subLabel, statusLabel, hintLabel;
    PanelChips panelChips;
    std::unique_ptr<ChipStack> engineChips;
    MiniKnob *splitKnob = nullptr, *balKnob = nullptr, *volKnob = nullptr,
             *tuneKnob = nullptr, *limitKnob = nullptr, *widthKnob = nullptr;
    std::unique_ptr<KeyZoneStrip> zoneStrip;
    std::unique_ptr<TouchMonitor> touchMonitor;
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

    // trackpad state
    eighty::TrackpadSource trackpad;
    eighty::GestureEngine gesture;
    bool trackpadAttached = false;
    bool strumKeyWasDown = false;
    bool strumLatched = false, armedPattern = false;

    static constexpr const char* noteKeys = "awsedftgyhujkolp;";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EightyEditor)
};
