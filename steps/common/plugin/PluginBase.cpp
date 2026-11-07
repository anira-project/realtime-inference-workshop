#include "PluginBase.h"

#include "PluginEditor.h"

#include <cmath>
#include <cstdio>
#include <exception>

namespace {
constexpr const char* k_mix_id = "mix";

// anira measured a plan slower than real time (steps 9 and 10): live it drops
// out; a bounce waits for every result, so it is not held back.
constexpr PluginBase::Hold k_not_realtime{
    "Not real-time",
    "anira measured this model slower than real time on this machine: live, it would drop "
    "out, and you would hear clicks. Run anyway plays it nonetheless.",
    "Run anyway, with dropouts",
    true};

// The host's rate is not the model's 44.1 kHz (steps 9 and 10): the model
// plays at the wrong speed, so it sounds higher or lower.
constexpr PluginBase::Hold k_not_44k{
    "44.1 kHz only",
    "The model is trained at 44.1 kHz: at another rate it sounds higher or lower. A "
    "resampler would fix it; this plugin has none. Process anyway plays it as it is.",
    "Process anyway",
    false};

// Step 8 at 48 kHz: the model and the step 4 number are 44.1 kHz ones.
constexpr PluginBase::Hold k_not_44k_step8{
    "44.1 kHz only",
    "The model is trained at 44.1 kHz, and your step 4 number is in 44.1 kHz samples: at "
    "48 kHz it sounds higher, and the worker's time is off. A resampler would fix it, but "
    "not with our fixed 512 and 2048 blocks: part 2, anira. Process anyway plays it as it is.",
    "Process anyway",
    false};

bool at_rate(double sample_rate, double rate) { return std::abs(sample_rate - rate) < 1.0; }

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

bool PluginBase::fixed_settings_fit(double sample_rate, size_t max_block_size) {
    // At 48 kHz the same samples last about 8% less: the worker's time,
    // rounded at 44.1 kHz, is off. Anything else is not run at all.
    const bool at_44k = at_rate(sample_rate, 44100.0);
    const bool at_48k = at_rate(sample_rate, 48000.0);
    if (!at_44k && !at_48k) {
        bypass("This plugin is built for 44.1 kHz. Arbitrary sample rates: part 2, anira.");
        return false;
    }
    if (max_block_size != 512) {
        bypass(
            "This plugin is built for buffers of 512 samples: set the audio device or the DAW "
            "to 512. Arbitrary buffer sizes: part 2, anira.");
        return false;
    }
    set_hold(HoldKind::sample_rate, at_48k ? &k_not_44k_step8 : nullptr);
    return true;
}

void PluginBase::ask_unless_44k(double sample_rate) {
    set_hold(HoldKind::sample_rate, at_rate(sample_rate, 44100.0) ? nullptr : &k_not_44k);
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

void PluginBase::prepareToPlay(double sample_rate, int samples_per_block) {
    setLatencySamples(0);
    release();  // The last prepare's threads stop, even if this one bypasses
    m_bypass.store(nullptr, std::memory_order_release);  // The step's prepare decides anew
    try {
        const size_t latency = prepare(sample_rate, static_cast<size_t>(samples_per_block));
        // The host delays every other track by this much
        setLatencySamples(bypassed() ? 0 : static_cast<int>(latency));
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        set_report({});  // Nothing runs: no figures to show
        bypass("The plugin could not prepare, see the console.");
    }
}

void PluginBase::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;
    render_source(buffer);
    if (bypassed()) { return; }  // Latency 0: the input goes out as it is

    float* mono = to_mono(buffer);
    process(mono, static_cast<size_t>(buffer.getNumSamples()));
    to_all_channels(buffer);
}

void PluginBase::render_source(juce::AudioBuffer<float>& buffer) {
    // A channel the host gave no input (a mono track into a stereo output)
    // holds what was there before, often our own last output: it gets the
    // input too, or silence without one.
    const int inputs = getTotalNumInputChannels();
    for (int channel = inputs; channel < buffer.getNumChannels(); ++channel) {
        if (inputs > 0) {
            buffer.copyFrom(channel, 0, buffer, 0, 0, buffer.getNumSamples());
        } else {
            buffer.clear(channel, 0, buffer.getNumSamples());
        }
    }

    if (!processing() && m_player.selected() != SourcePlayer::Source::input) {
        buffer.clear();
        m_player.rewind();
        return;
    }
    m_player.render(buffer);
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

namespace {
// What the host gets when a step's constructor throws: the window, bypassed,
// with the reason in the console.
class FailedPlugin final : public PluginBase {
public:
    FailedPlugin() : PluginBase("Workshop", "The plugin did not load") { bypass(k_reason); }

private:
    static constexpr const char* k_reason = "The model did not load, see the console.";
    size_t prepare(double, size_t) override {
        bypass(k_reason);
        return 0;
    }
    void process(float*, size_t) override {}
};
}  // namespace

juce::AudioProcessor* create_failed_plugin(const std::exception& error) {
    std::fprintf(stderr, "plugin: %s\n", error.what());
    return new FailedPlugin();
}
