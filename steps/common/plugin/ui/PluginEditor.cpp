#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr int k_width = 420;
constexpr int k_margin = 28;
constexpr int k_header = 76;
constexpr int k_gap = 12;
constexpr int k_load = 28;
constexpr int k_mix = 76;
constexpr int k_status = 24;
constexpr int k_knobs = 168;  // Only with knobs: value, slider, label

juce::Font font(float height, bool bold = false) {
    return juce::Font(juce::FontOptions(height, bold ? juce::Font::bold : juce::Font::plain));
}

bool is_audio_file(const juce::String& path) {
    return juce::File(path).hasFileExtension("wav;aif;aiff;flac");
}
}  // namespace

BypassOverlay::BypassOverlay() {
    addChildComponent(m_run_anyway);
    addChildComponent(m_show_report);
}

BypassOverlay::Layout BypassOverlay::layout() const {
    // The block, centred as a whole: the buttons only when held back
    const int buttons = m_held_back ? 12 + 32 : 0;
    auto area = getLocalBounds().reduced(k_margin * 2, 0);
    const int reason = 18 * m_reason_lines;
    area = area.withSizeKeepingCentre(area.getWidth(), 30 + reason + 20 + buttons);
    Layout layout;
    layout.m_title = area.removeFromTop(30);
    layout.m_reason = area.removeFromTop(reason);
    layout.m_host = area.removeFromTop(20);
    area.removeFromTop(m_held_back ? 12 : 0);
    layout.m_buttons = area.expanded(k_margin, 0);
    return layout;
}

void BypassOverlay::paint(juce::Graphics& g) {
    g.fillAll(palette::k_background.withAlpha(0.94f));
    const Layout area = layout();
    g.setColour(palette::k_warning);
    g.setFont(font(17.0f, true));
    g.drawText(m_title, area.m_title, juce::Justification::centred);
    g.setColour(palette::k_text);
    g.setFont(font(13.0f));
    g.drawFittedText(m_reason, area.m_reason, juce::Justification::centredTop, m_reason_lines);
    g.setColour(palette::k_muted);
    g.setFont(font(12.0f));
    g.drawFittedText(m_host, area.m_host, juce::Justification::centredTop, 1);
}

void BypassOverlay::resized() {
    auto row = layout().m_buttons;
    if (m_show_report.isVisible()) {
        m_run_anyway.setBounds(row.removeFromLeft(row.getWidth() * 2 / 3));
        m_show_report.setBounds(row);
    } else {
        m_run_anyway.setBounds(row);
    }
}

ReportPanel::ReportPanel() {
    addAndMakeVisible(m_close);
}

namespace {
juce::Colour verdict_colour(PluginBase::ReportRow::Verdict verdict) {
    using Verdict = PluginBase::ReportRow::Verdict;
    switch (verdict) {
        case Verdict::comfortable: return palette::k_good;
        case Verdict::marginal: return palette::k_marginal;
        case Verdict::not_realtime: return palette::k_warning;
        default: return palette::k_muted;
    }
}

const char* verdict_text(PluginBase::ReportRow::Verdict verdict) {
    using Verdict = PluginBase::ReportRow::Verdict;
    switch (verdict) {
        case Verdict::comfortable: return "comfortable";
        case Verdict::marginal: return "marginal";
        case Verdict::not_realtime: return "not real-time";
        default: return "unmeasured";
    }
}

juce::String ms(double value) {
    return juce::String(value, value < 10.0 ? 2 : 1) + " ms";
}
}  // namespace

void ReportPanel::paint(juce::Graphics& g) {
    g.fillAll(palette::k_background);
    auto area = getLocalBounds().reduced(k_margin, 0).withTrimmedTop(k_margin);

    g.setColour(palette::k_text);
    g.setFont(font(17.0f, true));
    g.drawText("anira's report", area.removeFromTop(22), juce::Justification::centredLeft);
    g.setColour(palette::k_muted);
    g.setFont(font(12.0f));
    g.drawText("Measured at prepare, on this machine, under its load of the moment",
               area.removeFromTop(18),
               juce::Justification::centredLeft);
    area.removeFromTop(k_gap);

    for (const auto& row : m_rows) {
        auto card = area.removeFromTop(104);
        area.removeFromTop(k_gap);
        g.setColour(palette::k_tile);
        g.fillRoundedRectangle(card.toFloat(), 12.0f);
        card = card.reduced(14, 10);

        // The model's name, and the verdict as a pill
        auto top = card.removeFromTop(20);
        g.setColour(palette::k_text);
        g.setFont(font(14.0f, true));
        g.drawText(row.m_name, top, juce::Justification::centredLeft);
        const juce::String verdict = verdict_text(row.m_verdict);
        const auto pill = top.removeFromRight(110).toFloat();
        g.setColour(verdict_colour(row.m_verdict));
        g.fillRoundedRectangle(pill, 10.0f);
        g.setColour(palette::k_background);
        g.setFont(font(12.0f, true));
        g.drawText(verdict, pill, juce::Justification::centred);

        // The times
        card.removeFromTop(6);
        g.setFont(font(12.0f));
        g.setColour(palette::k_text);
        g.drawText("budget " + ms(row.m_budget_ms) + "   measured " + ms(row.m_measured_ms) +
                       "   median " + ms(row.m_median_ms),
                   card.removeFromTop(16),
                   juce::Justification::centredLeft);
        g.setColour(palette::k_muted);
        g.drawText("warm-up: " + juce::String(row.m_warmup_inferences) + " inferences, " + row.m_warmup,
                   card.removeFromTop(16),
                   juce::Justification::centredLeft);

        // The load: the real-time factor shared by the inferences that run at
        // once; below 0.75 comfortable, below 1 marginal
        card.removeFromTop(6);
        auto load_line = card.removeFromTop(16);
        const double load = row.load();
        g.drawText("load " + juce::String(juce::roundToInt(load * 100.0)) + " %  (real-time factor " +
                       juce::String(row.m_rt_factor, 2) + " over " + juce::String(row.m_rt_parallel) +
                       (row.m_rt_parallel == 1 ? " inference)" : " inferences)"),
                   load_line,
                   juce::Justification::centredLeft);
        const auto bar = card.removeFromTop(8).toFloat();
        const float full = 1.25f;  // The bar's end, past 1
        g.setColour(palette::k_background);
        g.fillRoundedRectangle(bar, 4.0f);
        g.setColour(verdict_colour(row.m_verdict));
        g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * std::min(1.0f, static_cast<float>(load) / full)), 4.0f);
        g.setColour(palette::k_muted);
        for (const float mark : {0.75f, 1.0f}) {
            const float x = bar.getX() + bar.getWidth() * mark / full;
            g.drawLine(x, bar.getY() - 3.0f, x, bar.getBottom() + 3.0f, 1.5f);
        }
    }
}

void ReportPanel::resized() {
    m_close.setBounds(getLocalBounds().reduced(k_margin, 0).removeFromBottom(k_margin + 20).removeFromTop(28).removeFromRight(80));
}

void SourceTile::paintButton(juce::Graphics& g, bool highlighted, bool down) {
    const auto area = getLocalBounds().toFloat();
    const bool on = getToggleState();

    g.setColour(on ? palette::k_accent : palette::k_tile.brighter(highlighted ? 0.06f : 0.0f));
    g.fillRoundedRectangle(area, 14.0f);
    if (m_drop_target) {
        g.setColour(palette::k_accent);
        g.drawRoundedRectangle(area.reduced(1.0f), 14.0f, 2.0f);
    }

    const auto ink = on ? palette::k_background : palette::k_text;
    g.setColour(ink);
    g.setFont(font(15.0f, true));
    g.drawText(m_title, area.translated(0.0f, down ? 1.0f : 0.0f), juce::Justification::centred);

    g.setColour(ink.withAlpha(on ? 0.8f : 0.6f));
    g.setFont(font(12.0f));
    g.drawFittedText(getButtonText(),
                     area.reduced(10.0f, 10.0f).toNearestInt(),
                     juce::Justification::centredBottom,
                     1);
}

void WorkshopLookAndFeel::drawButtonText(juce::Graphics& g,
                                         juce::TextButton& button,
                                         bool highlighted,
                                         bool) {
    g.setColour(highlighted ? palette::k_text : palette::k_muted);
    g.setFont(font(12.0f));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

void WorkshopLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                           int x,
                                           int y,
                                           int width,
                                           int height,
                                           float position,
                                           float,
                                           float,
                                           juce::Slider::SliderStyle style,
                                           juce::Slider& slider) {
    if (style == juce::Slider::LinearVertical) {
        // Filled from 0 when the range holds it, else from the bottom
        const float middle = static_cast<float>(x) + static_cast<float>(width) / 2.0f;
        const auto track = juce::Rectangle<float>(middle - 4.0f,
                                                  static_cast<float>(y),
                                                  8.0f,
                                                  static_cast<float>(height));
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const float from = bipolar ? slider.getPositionOfValue(0.0) : track.getBottom();
        g.setColour(palette::k_tile);
        g.fillRoundedRectangle(track, 4.0f);
        g.setColour(palette::k_accent);
        g.fillRoundedRectangle(track.withTop(std::min(from, position)).withBottom(std::max(from, position)), 4.0f);
        g.setColour(palette::k_text);
        g.fillEllipse(juce::Rectangle<float>(18.0f, 18.0f).withCentre({middle, position}));
        return;
    }
    const float centre = static_cast<float>(y) + static_cast<float>(height) / 2.0f;
    const auto track = juce::Rectangle<float>(static_cast<float>(x),
                                              centre - 4.0f,
                                              static_cast<float>(width),
                                              8.0f);
    g.setColour(palette::k_tile);
    g.fillRoundedRectangle(track, 4.0f);
    g.setColour(palette::k_accent);
    g.fillRoundedRectangle(track.withRight(position), 4.0f);
    g.setColour(palette::k_text);
    g.fillEllipse(juce::Rectangle<float>(22.0f, 22.0f).withCentre({position, centre}));
}

PluginEditor::PluginEditor(PluginBase& processor)
    : juce::AudioProcessorEditor(processor), m_processor(processor) {
    setLookAndFeel(&m_look);

    m_example.m_title = "Example";
    m_file.m_title = "Your file";
    m_input.m_title = "Input";
    m_example.onClick = [this] { select(SourcePlayer::Source::example); };
    // Selects your file; once it plays, or before there is one, a click
    // chooses another.
    m_file.onClick = [this] {
        const auto& player = m_processor.player();
        if (player.has_file() && player.selected() != SourcePlayer::Source::file) {
            select(SourcePlayer::Source::file);
        } else {
            choose_file();
        }
    };
    m_input.onClick = [this] { select(SourcePlayer::Source::input); };
    m_load.onClick = [this] { choose_file(); };
    for (auto* component : {&m_example, &m_file, &m_input}) { addAndMakeVisible(component); }
    addAndMakeVisible(m_load);

    m_mix_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.parameters(),
        "mix",
        m_mix);
    m_mix.onValueChange = [this] { repaint(m_mix_area); };
    addAndMakeVisible(m_mix);

    for (const auto& knob : m_processor.knobs()) {
        auto& slider = *m_knobs.emplace_back(
            std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::NoTextBox));
        m_knob_attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            m_processor.parameters(),
            knob.m_id,
            slider));
        slider.setDoubleClickReturnValue(true, knob.m_default);  // A double-click resets it
        slider.onValueChange = [this] { repaint(m_knobs_area); };
        addAndMakeVisible(slider);
    }

    // anira's report: the button in the header, the panel over the window
    m_report_button.onClick = [this] { m_report.setVisible(true); };
    m_report.m_close.onClick = [this] { m_report.setVisible(false); };
    addChildComponent(m_report_button);
    m_overlay.m_run_anyway.onClick = [this] {
        m_processor.run_anyway();
        refresh();
    };
    m_overlay.m_show_report.onClick = [this] { m_report.setVisible(true); };

    addChildComponent(m_overlay);  // Last, so it lies over everything
    addChildComponent(m_report);

    const int tile = (k_width - 2 * k_margin - 2 * k_gap) / 3;
    setSize(k_width,
            k_header + tile + k_load + k_mix + (m_knobs.empty() ? 0 : k_knobs) + k_status + k_margin / 2);
    refresh();
    startTimerHz(10);
}

PluginEditor::~PluginEditor() {
    setLookAndFeel(nullptr);
}

void PluginEditor::paint(juce::Graphics& g) {
    g.fillAll(palette::k_background);

    auto header = getLocalBounds().reduced(k_margin, 0).withTrimmedTop(k_margin).removeFromTop(40);
    g.setColour(palette::k_text);
    g.setFont(font(17.0f, true));
    g.drawText(m_processor.title(), header.removeFromTop(22), juce::Justification::centredLeft);
    g.setColour(palette::k_muted);
    g.setFont(font(12.0f));
    g.drawText(m_processor.subtitle(), header, juce::Justification::centredLeft);

    const auto labels = m_mix_area.withHeight(16);
    g.setFont(font(12.0f));
    g.setColour(palette::k_muted);
    g.drawText("Dry", labels, juce::Justification::centredLeft);
    g.drawText("Wet", labels, juce::Justification::centredRight);
    g.setColour(palette::k_text);
    g.drawText(juce::String(juce::roundToInt(m_mix.getValue() * 100.0)) + " %",
               labels,
               juce::Justification::centred);

    // Each knob: its value above, its name below
    const auto& knobs = m_processor.knobs();
    for (size_t i = 0; i < m_knobs.size(); ++i) {
        const auto column = m_knobs[i]->getBounds().withY(m_knobs_area.getY()).withHeight(m_knobs_area.getHeight());
        g.setColour(palette::k_text);
        const double value = m_knobs[i]->getValue();
        g.drawText(juce::String(std::abs(value) < 0.005 ? 0.0 : value, 2),  // No "-0.00"
                   column.withHeight(16),
                   juce::Justification::centred);
        g.setColour(palette::k_muted);
        g.drawText(knobs[i].m_id, column.withTrimmedTop(column.getHeight() - 16), juce::Justification::centred);
    }

    if (m_status.isNotEmpty()) {
        g.setColour(palette::k_warning);
        g.drawText(m_status,
                   getLocalBounds().reduced(k_margin, 0).removeFromBottom(k_status + 8),
                   juce::Justification::centredLeft,
                   true);
    }
}

void PluginEditor::resized() {
    auto area = getLocalBounds().reduced(k_margin, 0).withTrimmedTop(k_header);
    const int tile = (area.getWidth() - 2 * k_gap) / 3;

    auto tiles = area.removeFromTop(tile + k_load);
    for (auto* source : {&m_example, &m_file, &m_input}) {
        auto column = tiles.removeFromLeft(tile);
        tiles.removeFromLeft(k_gap);
        source->setBounds(column.removeFromTop(tile));
        if (source == &m_file) { m_load.setBounds(column.withSizeKeepingCentre(60, k_load)); }
    }

    area.removeFromTop(16);
    m_mix_area = area.removeFromTop(k_mix - 16);
    m_mix.setBounds(m_mix_area.withTrimmedTop(20).withHeight(40));

    if (!m_knobs.empty()) {
        m_knobs_area = area.removeFromTop(k_knobs).withTrimmedTop(8);
        auto row = m_knobs_area.reduced(0, 22);  // Room for the value and the name
        const int column = row.getWidth() / static_cast<int>(m_knobs.size());
        for (auto& slider : m_knobs) { slider->setBounds(row.removeFromLeft(column).reduced(0, 9)); }
    }
    m_overlay.setBounds(getLocalBounds());
    m_report.setBounds(getLocalBounds());
    m_report_button.setBounds(getLocalBounds().reduced(k_margin, 0).withTrimmedTop(k_margin).removeFromTop(22).removeFromRight(60));
}

void PluginEditor::timerCallback() {
    if (m_processor.player().selected() != selected() ||
        overlay_key() != m_overlay_key ||
        m_processor.report_version() != m_report_version) {
        refresh();
    }
}

SourcePlayer::Source PluginEditor::selected() const {
    if (m_file.getToggleState()) { return SourcePlayer::Source::file; }
    if (m_input.getToggleState()) { return SourcePlayer::Source::input; }
    return SourcePlayer::Source::example;
}

void PluginEditor::refresh() {
    const auto& player = m_processor.player();
    const auto source = player.selected();
    m_example.setToggleState(source == SourcePlayer::Source::example, juce::dontSendNotification);
    m_file.setToggleState(source == SourcePlayer::Source::file, juce::dontSendNotification);
    m_input.setToggleState(source == SourcePlayer::Source::input, juce::dontSendNotification);
    m_example.setButtonText("example_1");
    m_file.setButtonText(player.has_file()
                             ? player.file_name().upToLastOccurrenceOf(".", false, false)
                             : "load or drop one");
    m_input.setButtonText("host or microphone");

    m_overlay.m_reason = overlay_reason();
    m_overlay_key = overlay_key();
    m_overlay.m_host = "The host: " + juce::String(m_processor.getSampleRate(), 0) + " Hz, " +
                       juce::String(m_processor.getBlockSize()) + " samples per buffer";
    // Held back by the report, not bypassed: then the user may run it anyway
    // Held back, not bypassed: the first hold that waits, with its button;
    // agreeing to it brings up the next
    const auto holds = m_processor.holds();
    m_overlay.m_held_back = m_processor.bypass_reason() == nullptr && !holds.empty();
    m_overlay.m_title = "Bypassed";
    if (m_overlay.m_held_back) {
        m_overlay.m_title = holds.front()->m_title;
        if (holds.size() > 1) { m_overlay.m_title << "  (1 of " << static_cast<int>(holds.size()) << ")"; }
        m_overlay.m_run_anyway.setButtonText(holds.front()->m_button);
    }
    m_overlay.m_run_anyway.setVisible(m_overlay.m_held_back);
    m_overlay.m_show_report.setVisible(m_overlay.m_held_back && !m_processor.report().empty());
    m_overlay.resized();  // The layout depends on m_held_back
    m_overlay.setVisible(m_overlay.m_reason.isNotEmpty());

    m_report_version = m_processor.report_version();
    m_report.m_rows = m_processor.report();
    m_report_button.setVisible(!m_report.m_rows.empty());
    m_report.repaint();
    m_overlay.repaint();
    repaint();
}

juce::String PluginEditor::overlay_reason() const {
    if (const char* bypass = m_processor.bypass_reason()) { return bypass; }
    const auto holds = m_processor.holds();
    return holds.empty() ? juce::String() : juce::String(holds.front()->m_reason);
}

juce::String PluginEditor::overlay_key() const {
    return overlay_reason() + "|" + juce::String(static_cast<int>(m_processor.holds().size()));
}

void PluginEditor::select(SourcePlayer::Source source) {
    m_processor.player().select(source);
    refresh();
}

void PluginEditor::choose_file() {
    // Opens where the last file came from.
    m_chooser = std::make_unique<juce::FileChooser>("Your audio file",
                                                    m_processor.player().file(),
                                                    "*.wav;*.aif;*.aiff;*.flac");
    m_chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            const juce::File file = chooser.getResult();
            if (file != juce::File{}) { load(file); }
        });
}

void PluginEditor::load(const juce::File& file) {
    const bool loaded = m_processor.load_file(file);
    m_status = loaded ? juce::String() : "cannot read " + file.getFileName();
    refresh();
}

bool PluginEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    return files.size() == 1 && is_audio_file(files[0]);
}

void PluginEditor::fileDragEnter(const juce::StringArray&, int, int) {
    m_file.m_drop_target = true;
    m_file.repaint();
}

void PluginEditor::fileDragExit(const juce::StringArray&) {
    m_file.m_drop_target = false;
    m_file.repaint();
}

void PluginEditor::filesDropped(const juce::StringArray& files, int, int) {
    fileDragExit(files);
    load(juce::File(files[0]));
}
