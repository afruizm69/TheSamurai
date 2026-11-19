#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
namespace SamuraiColours
{
    const juce::Colour black      { 0xff070807 };
    const juce::Colour panel      { 0xff101311 };
    const juce::Colour panelEdge  { 0xff1f2a22 };
    const juce::Colour green      { 0xff00e676 };
    const juce::Colour greenDark  { 0xff0b6b3a };
    const juce::Colour white      { 0xfff2f2f2 };
    const juce::Colour grey       { 0xff8a948d };
    const juce::Colour track      { 0xff26302a };
}

class SamuraiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SamuraiLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

//==============================================================================
struct Knob : public juce::Component
{
    Knob (APVTS& s, const juce::String& id, const juce::String& text);
    void resized() override;

    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

struct Combo : public juce::Component
{
    Combo (APVTS& s, const juce::String& id, const juce::String& text, int labelWidth = 18);
    void resized() override;

    juce::Label label;
    juce::ComboBox box;
    int labelW;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

struct Toggle : public juce::Component
{
    Toggle (APVTS& s, const juce::String& id, const juce::String& text);
    void resized() override { button.setBounds (getLocalBounds()); }

    juce::TextButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

//==============================================================================
class SourceSlot : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    SourceSlot (SamuraiProcessor& p, int slotIndex);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

    bool isInterestedInFileDragging (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true;  repaint(); }
    void fileDragExit  (const juce::StringArray&) override           { dragOver = false; repaint(); }
    void filesDropped  (const juce::StringArray& files, int, int) override;

private:
    void loadFile (const juce::File& f);

    SamuraiProcessor& proc;
    int slot;
    bool dragOver = false;
    juce::String error;

    juce::Label nameLabel;
    juce::TextButton loadButton { "LOAD" }, clearButton { "X" };
    Knob gain, offset;
    Toggle invert;
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
class WaveformView : public juce::Component
{
public:
    explicit WaveformView (APVTS& s);
    void setBuffer (const juce::AudioBuffer<float>& b) { buffer.makeCopyOf (b); repaint(); }
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    float msToX (float ms) const;
    float xToMs (float x) const;
    int hitBoundary (float x) const;

    APVTS& state;
    juce::RangedAudioParameter* punchParam;
    juce::RangedAudioParameter* bodyParam;
    juce::AudioBuffer<float> buffer;
    int dragging = 0;
};

//==============================================================================
class PartPanel : public juce::Component
{
public:
    PartPanel (APVTS& s, int partIndex);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int index;
    Combo srcA, srcB;
    Knob blend, level, hp, lp;
    Toggle solo, lock;
};

//==============================================================================
class DragHandle : public juce::Component
{
public:
    std::function<juce::File()> makeFile;
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { started = false; }

private:
    bool started = false;
};

//==============================================================================
class SamuraiEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SamuraiEditor (SamuraiProcessor&);
    ~SamuraiEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawPanel (juce::Graphics&, juce::Rectangle<int> area, const juce::String& title);

    SamuraiProcessor& proc;
    SamuraiLookAndFeel lnf;

    juce::TextButton playButton { "PLAY" }, exploreButton { "EXPLORE" }, backButton { "BACK" },
                     exportButton { "EXPORT" }, saveButton { "SAVE" }, openButton { "OPEN" };
    DragHandle dragHandle;

    juce::OwnedArray<SourceSlot> slots;
    WaveformView wave;
    juce::OwnedArray<PartPanel> parts;

    juce::OwnedArray<Combo> stageCombos;
    juce::OwnedArray<Knob> sculptKnobs, eqKnobs, outKnobs;
    juce::OwnedArray<Toggle> outToggles;

    juce::Rectangle<int> sculptArea, eqArea, outArea;
    std::unique_ptr<juce::FileChooser> chooser;
    int lastVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamuraiEditor)
};
