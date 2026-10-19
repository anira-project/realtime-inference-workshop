#include "PluginEditor.h"

namespace {
constexpr int k_width = 420;
constexpr int k_margin = 28;
constexpr int k_header = 76;
constexpr int k_gap = 12;
constexpr int k_load = 28;
constexpr int k_mix = 76;
constexpr int k_status = 24;

juce::Font font(float height, bool bold = false) {
    return juce::Font(juce::FontOptions(height, bold ? juce::Font::bold : juce::Font::plain));
}

bool is_audio_file(const juce::String& path) {
    return juce::File(path).hasFileExtension("wav;aif;aiff;flac");
}
}  // namespace

void SlotTile::paintButton(juce::Graphics& g, bool highlighted, bool down) {
    const auto area = getLocalBounds().toFloat();
    const bool on = getToggleState();

    g.setColour(on ? palette::k_accent : palette::k_tile.brighter(highlighted ? 0.06f : 0.0f));
    g.fillRoundedRectangle(area, 14.0f);
    if (m_drop_target) {
        g.setColour(palette::k_accent);
        g.drawRoundedRectangle(area.reduced(1.0f), 14.0f, 2.0f);
    }

    const auto ink = on ? palette::k_background : palette::k_text;
    g.setColour(ink.withAlpha(0.5f));
    g.setFont(font(12.0f));
    g.drawText(m_number, area.reduced(12.0f, 10.0f), juce::Justification::topLeft);

    // Play triangle or stop square, in the middle
    const auto centre = area.getCentre().translated(0.0f, down ? 1.0f : 0.0f);
    const float size = area.getWidth() * 0.11f;
    g.setColour(ink);
    if (on) {
        g.fillRoundedRectangle(juce::Rectangle<float>(1.6f * size, 1.6f * size).withCentre(centre), 3.0f);
    } else {
        juce::Path triangle;
        triangle.addTriangle(centre.x - 0.7f * size, centre.y - size,
                             centre.x - 0.7f * size, centre.y + size,
                             centre.x + size, centre.y);
        g.fillPath(triangle);
    }

    g.setColour(ink.withAlpha(on ? 0.8f : 0.6f));
    g.setFont(font(12.0f));
    g.drawFittedText(getButtonText(), area.reduced(12.0f, 10.0f).toNearestInt(),
                     juce::Justification::centredBottom, 1);
}

void WorkshopLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                         bool highlighted, bool) {
    g.setColour(highlighted ? palette::k_text : palette::k_muted);
    g.setFont(font(12.0f));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

void WorkshopLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float position, float, float, juce::Slider::SliderStyle,
                                           juce::Slider&) {
    const float centre = static_cast<float>(y) + static_cast<float>(height) / 2.0f;
    const auto track = juce::Rectangle<float>(static_cast<float>(x), centre - 4.0f,
                                              static_cast<float>(width), 8.0f);
    g.setColour(palette::k_tile);
    g.fillRoundedRectangle(track, 4.0f);
    g.setColour(palette::k_accent);
    g.fillRoundedRectangle(track.withRight(position), 4.0f);
    g.setColour(palette::k_text);
    g.fillEllipse(juce::Rectangle<float>(22.0f, 22.0f).withCentre({position, centre}));
}

WorkshopPluginEditor::WorkshopPluginEditor(WorkshopPluginProcessor& processor)
    : juce::AudioProcessorEditor(processor), m_processor(processor) {
    setLookAndFeel(&m_look);

    for (int slot = 0; slot < ExamplePlayer::k_num_slots; ++slot) {
        auto& row = m_slots[static_cast<size_t>(slot)];
        row.m_tile.m_number = juce::String(slot + 1);
        row.m_tile.onClick = [this, slot] {
            auto& player = m_processor.player();
            player.play(player.playing() == slot ? -1 : slot);
            refresh();
        };
        row.m_load.onClick = [this, slot] { choose_file(slot); };
        addAndMakeVisible(row.m_tile);
        addAndMakeVisible(row.m_load);
    }

    m_mix_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.parameters(), "mix", m_mix);
    m_mix.onValueChange = [this] { repaint(m_mix_area); };
    addAndMakeVisible(m_mix);

    const int tile = (k_width - 2 * k_margin - 2 * k_gap) / ExamplePlayer::k_num_slots;
    setSize(k_width, k_header + tile + k_load + k_mix + k_status + k_margin / 2);
    refresh();
    startTimerHz(10);
}

WorkshopPluginEditor::~WorkshopPluginEditor() {
    setLookAndFeel(nullptr);
}

void WorkshopPluginEditor::paint(juce::Graphics& g) {
    g.fillAll(palette::k_background);

    auto header = getLocalBounds().reduced(k_margin, 0).withTrimmedTop(k_margin).removeFromTop(40);
    g.setColour(palette::k_text);
    g.setFont(font(17.0f, true));
    g.drawText("Workshop Inference", header.removeFromTop(22), juce::Justification::centredLeft);
    g.setColour(palette::k_muted);
    g.setFont(font(12.0f));
    g.drawText("Realtime Audio Variational autoEncoder",
               header, juce::Justification::centredLeft);

    const auto labels = m_mix_area.withHeight(16);
    g.setFont(font(12.0f));
    g.setColour(palette::k_muted);
    g.drawText("Dry", labels, juce::Justification::centredLeft);
    g.drawText("Wet", labels, juce::Justification::centredRight);
    g.setColour(palette::k_text);
    g.drawText(juce::String(juce::roundToInt(m_mix.getValue() * 100.0)) + " %", labels,
               juce::Justification::centred);

    if (m_status.isNotEmpty()) {
        g.setColour(palette::k_warning);
        g.drawText(m_status, getLocalBounds().reduced(k_margin, 0).removeFromBottom(k_status + 8),
                   juce::Justification::centredLeft, true);
    }
}

void WorkshopPluginEditor::resized() {
    auto area = getLocalBounds().reduced(k_margin, 0).withTrimmedTop(k_header);
    const int tile = (area.getWidth() - 2 * k_gap) / ExamplePlayer::k_num_slots;

    auto tiles = area.removeFromTop(tile + k_load);
    for (auto& row : m_slots) {
        auto column = tiles.removeFromLeft(tile);
        tiles.removeFromLeft(k_gap);
        row.m_tile.setBounds(column.removeFromTop(tile));
        row.m_load.setBounds(column.withSizeKeepingCentre(60, k_load));
    }

    area.removeFromTop(16);
    m_mix_area = area.removeFromTop(k_mix - 16);
    m_mix.setBounds(m_mix_area.withTrimmedTop(20).withHeight(40));
}

void WorkshopPluginEditor::timerCallback() {
    const int playing = m_processor.player().playing();
    const juce::String status = m_processor.load_error();
    if (playing != m_playing || status != m_status) { refresh(); }
}

void WorkshopPluginEditor::refresh() {
    m_playing = m_processor.player().playing();
    m_status = m_processor.load_error();
    for (int slot = 0; slot < ExamplePlayer::k_num_slots; ++slot) {
        auto& tile = m_slots[static_cast<size_t>(slot)].m_tile;
        tile.setToggleState(m_playing == slot, juce::dontSendNotification);
        tile.setButtonText(m_processor.player().name(slot).upToLastOccurrenceOf(".", false, false));
    }
    repaint();
}

void WorkshopPluginEditor::choose_file(int slot) {
    m_chooser = std::make_unique<juce::FileChooser>(
        "Audio file for slot " + juce::String(slot + 1), juce::File{}, "*.wav;*.aif;*.aiff;*.flac");
    m_chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, slot](const juce::FileChooser& chooser) {
            const juce::File file = chooser.getResult();
            if (file != juce::File{}) { load(slot, file); }
        });
}

void WorkshopPluginEditor::load(int slot, const juce::File& file) {
    const bool loaded = m_processor.load_example(slot, file);
    refresh();
    if (!loaded) {
        m_status = "cannot read " + file.getFileName();
        repaint();
    }
}

int WorkshopPluginEditor::slot_at(int x, int y) const {
    for (int slot = 0; slot < ExamplePlayer::k_num_slots; ++slot) {
        if (m_slots[static_cast<size_t>(slot)].m_tile.getBounds().contains(x, y)) { return slot; }
    }
    return -1;
}

bool WorkshopPluginEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    return files.size() == 1 && is_audio_file(files[0]);
}

void WorkshopPluginEditor::fileDragMove(const juce::StringArray&, int x, int y) {
    const int target = slot_at(x, y);
    for (int slot = 0; slot < ExamplePlayer::k_num_slots; ++slot) {
        auto& tile = m_slots[static_cast<size_t>(slot)].m_tile;
        if (tile.m_drop_target != (slot == target)) {
            tile.m_drop_target = slot == target;
            tile.repaint();
        }
    }
}

void WorkshopPluginEditor::fileDragExit(const juce::StringArray& files) {
    fileDragMove(files, -1, -1);
}

void WorkshopPluginEditor::filesDropped(const juce::StringArray& files, int x, int y) {
    fileDragExit(files);
    const int slot = slot_at(x, y);
    if (slot >= 0) { load(slot, juce::File(files[0])); }
}
