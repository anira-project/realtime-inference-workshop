// The plugin's window: three example tiles, the dry/wet mix, and what went
// wrong if the model did not load. Colours from the slides.
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <memory>

#include "PluginProcessor.h"

namespace palette {
const juce::Colour k_background{0xff1c1d21};
const juce::Colour k_tile{0xff2a2b30};
const juce::Colour k_text{0xfffafafa};
const juce::Colour k_muted{0xff8a8a90};
const juce::Colour k_accent{0xffbf85fc};
const juce::Colour k_warning{0xfffe938c};
}  // namespace palette

// One example slot: the whole tile plays and stops it.
class SlotTile : public juce::Button {
public:
    SlotTile() : juce::Button({}) {}
    void paintButton(juce::Graphics& g, bool highlighted, bool down) override;

    juce::String m_number;
    bool m_drop_target = false;
};

// Text-only buttons and a thin mix slider.
class WorkshopLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override {}
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float position,
                          float, float, juce::Slider::SliderStyle, juce::Slider&) override;
};

class WorkshopPluginEditor : public juce::AudioProcessorEditor,
                             public juce::FileDragAndDropTarget,
                             private juce::Timer {
public:
    explicit WorkshopPluginEditor(WorkshopPluginProcessor& processor);
    ~WorkshopPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // An audio file dropped on a tile replaces that slot.
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    struct Slot {
        SlotTile m_tile;
        juce::TextButton m_load{"load"};
    };

    void timerCallback() override;
    void choose_file(int slot);
    void load(int slot, const juce::File& file);
    int slot_at(int x, int y) const;
    void refresh();

    WorkshopPluginProcessor& m_processor;
    WorkshopLookAndFeel m_look;
    std::array<Slot, ExamplePlayer::k_num_slots> m_slots;
    juce::Slider m_mix{juce::Slider::LinearHorizontal, juce::Slider::NoTextBox};
    juce::Rectangle<int> m_mix_area;
    juce::String m_status;
    int m_playing = -1;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_mix_attachment;
    std::unique_ptr<juce::FileChooser> m_chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkshopPluginEditor)
};
