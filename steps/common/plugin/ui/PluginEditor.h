// The plugin's window: what to process (the example, your file, or the
// input), the dry/wet mix, and an overlay while the plugin is bypassed.
// Colours from the slides.
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>

#include "PluginBase.h"

namespace palette {
const juce::Colour k_background{0xff1c1d21};
const juce::Colour k_tile{0xff2a2b30};
const juce::Colour k_text{0xfffafafa};
const juce::Colour k_muted{0xff8a8a90};
const juce::Colour k_accent{0xffbf85fc};
const juce::Colour k_warning{0xfffe938c};
}  // namespace palette

// Over the whole window while the plugin is bypassed: why, and what to change.
class BypassOverlay : public juce::Component {
public:
    void paint(juce::Graphics& g) override;

    juce::String m_reason;
    juce::String m_host;  // The host's sample rate and buffer size
};

// One source: the whole tile selects it.
class SourceTile : public juce::Button {
public:
    SourceTile() : juce::Button({}) {}
    void paintButton(juce::Graphics& g, bool highlighted, bool down) override;

    juce::String m_title;
    bool m_drop_target = false;
};

// Text-only buttons and a thin mix slider.
class WorkshopLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawButtonBackground(juce::Graphics&,
                              juce::Button&,
                              const juce::Colour&,
                              bool,
                              bool) override {}
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawLinearSlider(juce::Graphics&,
                          int x,
                          int y,
                          int width,
                          int height,
                          float position,
                          float,
                          float,
                          juce::Slider::SliderStyle,
                          juce::Slider&) override;
};

class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::FileDragAndDropTarget,
                     private juce::Timer {
public:
    explicit PluginEditor(PluginBase& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // An audio file dropped on the window becomes your file.
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void select(SourcePlayer::Source source);
    SourcePlayer::Source selected() const;  // What the tiles show
    void choose_file();
    void load(const juce::File& file);
    void refresh();

    PluginBase& m_processor;
    WorkshopLookAndFeel m_look;
    SourceTile m_example;
    SourceTile m_file;
    SourceTile m_input;
    juce::TextButton m_load{"load"};
    juce::Slider m_mix{juce::Slider::LinearHorizontal, juce::Slider::NoTextBox};
    juce::Rectangle<int> m_mix_area;
    juce::String m_status;  // A file that could not be read
    BypassOverlay m_overlay;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_mix_attachment;
    std::unique_ptr<juce::FileChooser> m_chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
