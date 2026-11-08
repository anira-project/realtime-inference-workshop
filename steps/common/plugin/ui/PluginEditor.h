// The plugin's window: what to process (the example, your file, or the
// input), the dry/wet mix, a row of the step's own knobs if it has any, an
// overlay while the plugin is bypassed or held back, and anira's plan report
// (step 9 and the bonus).
// Colours from the slides.
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>
#include <vector>

#include "PluginBase.h"

namespace palette {
const juce::Colour k_background{0xff1c1d21};
const juce::Colour k_tile{0xff2a2b30};
const juce::Colour k_text{0xfffafafa};
const juce::Colour k_muted{0xff8a8a90};
const juce::Colour k_accent{0xffbf85fc};
const juce::Colour k_warning{0xfffe938c};
const juce::Colour k_good{0xff8fd6a0};
const juce::Colour k_marginal{0xfff2c46d};
}  // namespace palette

// Over the whole window while the plugin is bypassed: why, and what to change.
// Held back by anira's report, it offers to try nonetheless and to show the report.
class BypassOverlay : public juce::Component {
public:
    BypassOverlay();
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Top to bottom, one after the other: title, reason, host line, buttons
    struct Layout {
        juce::Rectangle<int> m_title, m_reason, m_host, m_buttons;
    };
    Layout layout() const;

    juce::String m_reason;
    juce::String m_host;  // The host's sample rate and buffer size
    juce::String m_title{"Bypassed"};
    int m_reason_lines = 6;   // Room for the reason
    bool m_held_back = false;
    juce::TextButton m_run_anyway{"Try nonetheless"};
    juce::TextButton m_show_report{"Report"};
};

// Over the whole window: anira's plan report, one card per model.
class ReportPanel : public juce::Component {
public:
    ReportPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<PluginBase::ReportRow> m_rows;
    juce::TextButton m_close{"Close"};
};

// One source: the whole tile selects it.
class SourceTile : public juce::Button {
public:
    SourceTile() : juce::Button({}) {}
    void paintButton(juce::Graphics& g, bool highlighted, bool down) override;

    juce::String m_title;
    bool m_drop_target = false;
};

// Text-only buttons, a thin mix slider, and thin vertical knob sliders,
// filled from 0 when the range holds it.
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
    // What the overlay says: the bypass, else the first hold that waits.
    // Empty: no overlay.
    juce::String overlay_reason() const;
    // Changes whenever the overlay has to: its reason, and how many holds wait.
    juce::String overlay_key() const;
    juce::String m_overlay_key;

    PluginBase& m_processor;
    WorkshopLookAndFeel m_look;
    SourceTile m_example;
    SourceTile m_file;
    SourceTile m_input;
    juce::TextButton m_load{"load"};
    juce::TextButton m_report_button{"report"};
    uint32_t m_report_version = 0;
    juce::Slider m_mix{juce::Slider::LinearHorizontal, juce::Slider::NoTextBox};
    juce::Rectangle<int> m_mix_area;
    std::vector<std::unique_ptr<juce::Slider>> m_knobs;  // The step's own, in PluginBase's order
    juce::Rectangle<int> m_knobs_area;
    juce::String m_status;  // A file that could not be read
    BypassOverlay m_overlay;
    ReportPanel m_report;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_mix_attachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> m_knob_attachments;
    std::unique_ptr<juce::FileChooser> m_chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
