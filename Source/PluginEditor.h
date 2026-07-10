#pragma once
#include "PluginProcessor.h"

// ------------------------------------------------- palette: "Cream Strip"
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
    const juce::Colour scopeTrace{ 0xffe8a33d };
    const juce::Colour jpAccent  { 0xff5f57a8 };   // NEW: Jupiter-8 identity

    // section stripes
    const juce::Colour stLfo   { 0xffb6412f };
    const juce::Colour stOsc   { 0xffc97f2e };
    const juce::Colour stMix   { 0xffa08a2f };
    const juce::Colour stFilt  { 0xff3f6ea5 };
    const juce::Colour stEnv   { 0xff4a8a58 };
    const juce::Colour stTouch { 0xff7d5a9e };
    const juce::Colour stVoice { 0xff3d8f83 };

    juce::Font sans (float size, bool bold = false);
    juce::Font mono (float size);
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
class VFader : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit VFader (const juce::String& labelText);
    void resized() override;
    LearnSlider slider;
    juce::Label label;
};

class MiniKnob : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MiniKnob (const juce::String& labelText);
    void resized() override;
    LearnSlider slider;
    juce::Label label;
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

private:
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

// ----------------------------------------------------------------- section
// Flat strip section: hairline left border, name + colored underline,
// LED toggles top-right, bottom-aligned content items.
class Section : public juce::Component
{
public:
    Section (const juce::String& name, juce::Colour stripe);
    void paint (juce::Graphics&) override;
    void resized() override;
    void addItem (juce::Component& c, int width);
    void addHeaderToggle (LedToggle& t);
    int preferredWidth() const;
    void setStripe (juce::Colour c) { stripe = c; repaint(); }

private:
    juce::String name;
    juce::Colour stripe;
    struct Item { juce::Component* comp; int width; };
    std::vector<Item> items;
    std::vector<LedToggle*> headerToggles;
};

// ------------------------------------------------------------------ scope
class ScopeComponent : public juce::Component, private juce::Timer
{
public:
    explicit ScopeComponent (ScopeFifo& fifo);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    ScopeFifo& fifo;
    std::vector<float> history, display;
    int writePos = 0;
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

// ------------------------------------------------- panel view chip switch
class PanelChips : public juce::Component
{
public:
    std::function<void (bool)> onSelect;   // true = JP panel
    void setJp (bool jp) { jpSel = jp; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    bool jpSel = false;
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
    void showAddMenu (bool instruments);
    void openWindowFor (juce::AudioPluginInstance*);
    void closeWindowFor (juce::AudioPluginInstance*);

    EightyProcessor& proc;
    juce::TextButton addBtn { "+ FX" }, synthBtn { "+ SET SYNTH" },
                     synthClearBtn { "x" };
    juce::OwnedArray<juce::TextButton> openBtns, removeBtns;
    juce::OwnedArray<juce::DocumentWindow> windows;
    LearnSlider synthLevel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> synthLevelAtt;
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
    void layoutRows();

    struct Ctl { juce::Component* target; juce::String paramID; };
    void selectControl (int index);
    void moveSelection (int dir);
    void adjustSelected (int dir);
    void updateHalo();

    VFader*   makeFader (Section&, const juce::String& paramID, const juce::String& label);
    MiniKnob* makeKnob  (Section&, const juce::String& paramID, const juce::String& label);
    ChipStack* makeChips (Section&, const juce::String& paramID,
                          juce::StringArray labels, const juce::String& group, int width = 44);
    LedToggle* makeLed  (Section&, const juce::String& paramID, const juce::String& label);
    MiniKnob* makeHeaderKnob (const juce::String& paramID, const juce::String& label);

    EightyProcessor& proc;
    CreamLNF lnf;

    // shared sections
    Section secLfo   { "LFO",       ui::stLfo },
            secMix   { "MIX",       ui::stMix },
            secTouch { "TOUCH",     ui::stTouch },
            secVoice { "VOICES",    ui::stVoice },
            secArp   { "ARP / SEQ", ui::stLfo },
            secFx    { "EFFECTS",   ui::stFilt };
    // CS-80 sections
    Section secOsc1  { "OSC I",      ui::stOsc },
            secOsc2  { "OSC II",     ui::stOsc },
            secFilter{ "FILTER",     ui::stFilt },
            secFEnv  { "FILTER ENV", ui::stEnv },
            secAEnv  { "AMP ENV",    ui::stEnv };
    // Jupiter-8 sections (unified JP accent stripe)
    Section secVco1    { "VCO 1",      ui::jpAccent },
            secVco2    { "VCO 2",      ui::jpAccent },
            secJpFilter{ "FILTER 12/24", ui::jpAccent },
            secJpFEnv  { "FILTER ENV",   ui::jpAccent },
            secJpAEnv  { "AMP ENV",      ui::jpAccent };

    ScopeComponent scope;
    PitchWheel wheel;
    InsertPanel insertPanel;
    juce::MidiKeyboardComponent keyboard;
    juce::Label titleLabel, subLabel, statusLabel, hintLabel;
    PanelChips panelChips;
    std::unique_ptr<ChipStack> engineChips;
    MiniKnob *splitKnob = nullptr, *balKnob = nullptr,
             *volKnob = nullptr, *tuneKnob = nullptr;
    std::unique_ptr<LedToggle> csLowLed;
    SelectionHalo halo;
    juce::TooltipWindow tooltipWindow { this, 400 };
    bool jpView = false;

    juce::OwnedArray<juce::Component> owned;   // faders, knobs, chips, leds
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtts;

    std::vector<Ctl> controls;
    int selectedCtl = -1;
    int lastEngineMode = -1;

    // footer readout
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
