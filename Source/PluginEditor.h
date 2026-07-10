#pragma once
#include "PluginProcessor.h"

// ---------------------------------------------------------------- palette
namespace ui
{
    const juce::Colour bg        { 0xff14161a };
    const juce::Colour panel     { 0xff1d2027 };
    const juce::Colour panelLine { 0xff2a2e38 };
    const juce::Colour text      { 0xffc9ced6 };
    const juce::Colour dimText   { 0xff7d8494 };
    const juce::Colour accent    { 0xffe8a33d };   // warm amber (CS-80)
    const juce::Colour accent2   { 0xff4fb8a8 };   // teal (Jupiter-8)
    const juce::Colour learn     { 0xffe85d5d };
}

// ------------------------------------------------------------ LookAndFeel
class EightyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EightyLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isDown,
                       int, int, int, int, juce::ComboBox&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool highlighted, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
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

// ------------------------------------------------------------------ knob
class Knob : public juce::Component,
             public juce::SettableTooltipClient
{
public:
    explicit Knob (const juce::String& labelText);
    void resized() override;
    LearnSlider slider;
    juce::Label label;
};

// --------------------------------------------------------------- section
class Section : public juce::Component
{
public:
    Section (const juce::String& title, juce::Colour accentColour = ui::accent);
    void paint (juce::Graphics&) override;
    void resized() override;
    void addItem (juce::Component& c, int cellSpan = 1);
    int columns = 4;

private:
    juce::String title;
    juce::Colour accentCol;
    struct Item { juce::Component* comp; int span; };
    std::vector<Item> items;
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
    std::vector<float> history;
    std::vector<float> display;
    int writePos = 0;
};

// ------------------------------------------------------------ pitch wheel
class PitchWheel : public juce::Component,
                   public juce::SettableTooltipClient
{
public:
    std::function<void (float, bool)> onChange;  // value -1..1, active
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setExternalValue (float v);             // for arrow-key bends

private:
    void setFromMouse (const juce::MouseEvent&);
    float value = 0.f;
};

// ------------------------------------------- keyboard-selection halo ring
class SelectionHalo : public juce::Component
{
public:
    SelectionHalo() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (ui::accent.withAlpha (0.9f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 6.f, 2.f);
    }
};

// ------------------------------------------- external VST3 insert panel
class InsertPanel : public juce::Component, private juce::Timer
{
public:
    explicit InsertPanel (EightyProcessor&);
    ~InsertPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refresh();
    void showAddMenu();
    void openEditorWindow (int index);
    void removeInsert (int index);

    EightyProcessor& proc;
    juce::TextButton addBtn { "+ ADD VST3" };
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

private:
    void timerCallback() override;
    void scanNoteKeys();
    void handleActionKey (juce::juce_wchar c);
    void showLearnMenu (const juce::String& paramID, juce::Point<int> screenPos);
    void setEngineView (bool jupiter);

    // arrow-key control selection
    struct Ctl { juce::Component* target; juce::String paramID; };
    void selectControl (int index);
    void moveSelection (int dir);
    void adjustSelected (int dir);
    void updateHalo();

    Knob* makeKnob (Section& s, const juce::String& paramID,
                    const juce::String& label, int span = 1);
    juce::ComboBox* makeCombo (Section& s, const juce::String& paramID,
                               const juce::String& label, int span = 1);
    juce::ToggleButton* makeToggle (Section& s, const juce::String& paramID,
                                    const juce::String& label, int span = 1);

    EightyProcessor& proc;
    EightyLookAndFeel lnf;

    // shared sections
    Section secCommon{ "COMMON" },
            secLfo   { "LFO" },
            secTouch { "TOUCH" },
            secEngine{ "ENGINE" },
            secVoice { "VOICES" },
            secArp   { "ARP / SEQ" },
            secFx    { "EFFECTS" };
    // CS-80 sections
    Section secOsc1  { "OSC I" },
            secOsc2  { "OSC II" },
            secFilter{ "FILTER  ·  HPF > LPF" },
            secFEnv  { "FILTER ENV" },
            secAEnv  { "AMP ENV" };
    // Jupiter-8 sections
    Section secVco1    { "VCO 1", ui::accent2 },
            secVco2    { "VCO 2", ui::accent2 },
            secJpFilter{ "FILTER  ·  12/24 dB", ui::accent2 },
            secJpFEnv  { "FILTER ENV", ui::accent2 },
            secJpAEnv  { "AMP ENV", ui::accent2 };

    ScopeComponent scope;
    PitchWheel wheel;
    InsertPanel insertPanel;
    juce::MidiKeyboardComponent keyboard;
    juce::Label titleLabel, chipLabel, statusLabel, hintLabel;
    juce::TextButton viewCsBtn { "CS-80" }, viewJpBtn { "JP-8" };
    SelectionHalo halo;
    juce::TooltipWindow tooltipWindow { this, 400 };
    bool jpView = false;

    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<juce::ToggleButton> toggles;
    juce::OwnedArray<juce::Component> wrappers;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAtts;

    std::vector<Ctl> controls;
    int selectedCtl = -1;
    int lastEngineMode = -1;

    // computer-keyboard playing state
    int baseNote = 48;            // C3
    float typeVelocity = 0.8f;
    bool noteKeyDown[32] {};
    float keyBend = 0.f;
    int bendDir = 0;
    bool wheelActive = false;
    int learnFlashCC = -1;

    static constexpr const char* noteKeys = "awsedftgyhujkolp;";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EightyEditor)
};
