#include "PluginProcessor.h"

#include <cstdio>

#include "PluginEditor.h"

namespace {
constexpr const char* k_mix_id = "mix";
}

WorkshopPluginProcessor::WorkshopPluginProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , m_parameters(*this, nullptr, "state", make_layout()) {
    m_mix = m_parameters.getRawParameterValue(k_mix_id);
}

WorkshopPluginProcessor::~WorkshopPluginProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout WorkshopPluginProcessor::make_layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{k_mix_id, 1},
                                                           "Dry/Wet",
                                                           0.0f,
                                                           1.0f,
                                                           1.0f));
    return layout;
}

// The host says how it will call us. Everything that allocates belongs here,
// including loading the model and starting the worker threads.
void WorkshopPluginProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    const auto channels = static_cast<size_t>(getTotalNumInputChannels());
    m_status.clear();
    m_prepared.store(false, std::memory_order_release);
    m_wrong_block.store(false, std::memory_order_relaxed);

    // Loading takes a second and the host may call this often — once per
    // channel is enough, and only when the channel count actually changed.
    try {
        while (m_engines.size() < channels) {
            m_engines.push_back(std::make_unique<OnnxEngine>(WORKSHOP_ONNX_MODEL_PATH));
            m_channels.push_back(std::make_unique<LatencyProcessor<OnnxEngine>>(*m_engines.back()));
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        m_status = error.what();
        m_engines.clear();
        m_channels.clear();
        setLatencySamples(0);
        return;
    }

    // The prefill and the latency of steps 5 and 6 are only right for one host
    // block size. Any other one needs a different design: that is part 2.
    const auto block_size = static_cast<size_t>(samples_per_block);
    if (!LatencyProcessor<OnnxEngine>::supports_block_size(block_size)) {
        for (auto& channel : m_channels) { channel->stop(); }
        m_status = "Only a " + juce::String(static_cast<int>(k_host_block_size)) +
                   "-sample buffer is supported, the host asks for " +
                   juce::String(samples_per_block) +
                   ". Arbitrary buffer sizes need a different design: part 2, anira.";
        setLatencySamples(0);
        return;
    }

    // Buffers and threads do get rebuilt.
    for (size_t channel = 0; channel < channels; ++channel) {
        m_channels[channel]->prepare(block_size);
    }
    m_prepared.store(true, std::memory_order_release);

    // The number from step 6, now where it belongs: the host delays every other
    // track by this much, so our output lines up with the rest of the session.
    setLatencySamples(static_cast<int>(LatencyProcessor<OnnxEngine>::latency_samples()));

    // The model was trained at one rate. Running it at another is a different
    // instrument, not a bug — worth hearing, worth knowing.
    if (std::abs(sample_rate - k_model.m_sample_rate) > 1.0) {
        m_status = "running at " + juce::String(sample_rate, 0) + " Hz, the model expects " +
                   juce::String(k_model.m_sample_rate, 0) + " Hz";
    }
}

juce::String WorkshopPluginProcessor::status() const {
    juce::String status = m_status;
    if (m_wrong_block.load(std::memory_order_relaxed)) {
        if (status.isNotEmpty()) { status += " · "; }
        status += "The host sent a block that is not " +
                  juce::String(static_cast<int>(k_host_block_size)) +
                  " samples: it went out as silence. Arbitrary buffer sizes need a different "
                  "design: part 2, "
                  "anira.";
    }
    return status;
}

void WorkshopPluginProcessor::releaseResources() {
    for (auto& channel : m_channels) { channel->stop(); }
}

// The audio callback. Everything in here comes from steps 5 and 6: no engine,
// no allocation, no lock — only the hand-over.
void WorkshopPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;

    const auto num_samples = static_cast<size_t>(buffer.getNumSamples());
    const auto channels = static_cast<size_t>(getTotalNumInputChannels());

    if (!m_prepared.load(std::memory_order_acquire) || m_channels.size() < channels) {
        buffer.clear();  // The model did not load, or the block size is not supported: silence
                         // beats garbage
        return;
    }
    if (num_samples == 0) { return; }
    if (!LatencyProcessor<OnnxEngine>::supports_block_size(num_samples)) {
        m_wrong_block.store(true, std::memory_order_relaxed);  // The editor says so
        buffer.clear();
        return;
    }

    // A playing example slot replaces the input.
    m_player.render(buffer);

    const float mix = m_mix != nullptr ? m_mix->load() : 1.0f;

    for (size_t channel = 0; channel < channels; ++channel) {
        m_channels[channel]->set_mix(mix);
        m_channels[channel]->process_block(buffer.getWritePointer(static_cast<int>(channel)),
                                           num_samples);
    }
}

bool WorkshopPluginProcessor::load_example(int slot, const juce::File& file) {
    suspendProcessing(true);
    const bool loaded = m_player.load(slot, file);
    suspendProcessing(false);
    return loaded;
}

juce::AudioProcessorEditor* WorkshopPluginProcessor::createEditor() {
    return new WorkshopPluginEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new WorkshopPluginProcessor();
}
