#include "PluginBase.h"

#include "PluginEditor.h"

namespace {
constexpr const char* k_mix_id = "mix";

juce::AudioProcessorValueTreeState::ParameterLayout make_layout(const std::vector<PluginBase::Knob>& knobs) {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{k_mix_id, 1},
                                                           "Dry/Wet",
                                                           0.0f,
                                                           1.0f,
                                                           1.0f));
    for (const auto& knob : knobs) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{knob.m_id, 1},
                                                               knob.m_id,
                                                               knob.m_min,
                                                               knob.m_max,
                                                               knob.m_default));
    }
    return layout;
}
}  // namespace

PluginBase::PluginBase(juce::String title, juce::String subtitle, std::vector<Knob> knobs)
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , m_title(std::move(title))
    , m_subtitle(std::move(subtitle))
    , m_knobs(std::move(knobs))
    , m_parameters(*this, nullptr, "state", make_layout(m_knobs)) {
    m_mix = m_parameters.getRawParameterValue(k_mix_id);
    for (const auto& knob : m_knobs) { m_knob_values.push_back(m_parameters.getRawParameterValue(knob.m_id)); }
}

PluginBase::~PluginBase() = default;

std::vector<const PluginBase::Hold*> PluginBase::holds() const {
    std::vector<const Hold*> open;
    for (size_t kind = 0; kind < 2; ++kind) {
        if (waiting(kind)) { open.push_back(m_holds[kind].load()); }
    }
    return open;
}

void PluginBase::run_anyway() {
    for (size_t kind = 0; kind < 2; ++kind) {
        if (waiting(kind)) {
            m_agreed[kind].store(true);
            return;  // One at a time: the window asks for the next
        }
    }
}

void PluginBase::set_report(std::vector<ReportRow> rows) {
    bool not_realtime = false;
    for (const ReportRow& row : rows) {
        not_realtime = not_realtime || row.m_verdict == ReportRow::Verdict::not_realtime;
    }
    {
        const std::lock_guard<std::mutex> lock(m_report_mutex);
        m_report = std::move(rows);
    }
    set_hold(HoldKind::report, not_realtime ? &k_not_realtime : nullptr);
    m_report_version.fetch_add(1);
}

void PluginBase::set_hold(HoldKind kind, const Hold* hold) {
    const size_t index = kind == HoldKind::sample_rate ? 0 : 1;
    if (m_holds[index].load() != hold) { m_agreed[index].store(false); }  // New: asks again
    m_holds[index].store(hold);
}

std::vector<PluginBase::ReportRow> PluginBase::report() const {
    const std::lock_guard<std::mutex> lock(m_report_mutex);
    return m_report;
}

bool PluginBase::load_file(const juce::File& file) {
    suspendProcessing(true);
    const bool loaded = m_player.load(file);
    suspendProcessing(false);
    if (loaded) { m_player.select(SourcePlayer::Source::file); }
    return loaded;
}

float* PluginBase::to_mono(juce::AudioBuffer<float>& buffer) const {
    const int num_samples = buffer.getNumSamples();
    const int channels = getTotalNumInputChannels();
    float* mono = buffer.getWritePointer(0);
    if (channels == 0) {
        juce::FloatVectorOperations::clear(mono, num_samples);
        return mono;
    }
    for (int channel = 1; channel < channels; ++channel) {
        juce::FloatVectorOperations::add(mono, buffer.getReadPointer(channel), num_samples);
    }
    juce::FloatVectorOperations::multiply(mono, 1.0f / static_cast<float>(channels), num_samples);
    return mono;
}

void PluginBase::to_all_channels(juce::AudioBuffer<float>& buffer) const {
    for (int channel = 1; channel < getTotalNumOutputChannels(); ++channel) {
        buffer.copyFrom(channel, 0, buffer, 0, 0, buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* PluginBase::createEditor() {
    return new PluginEditor(*this);
}
