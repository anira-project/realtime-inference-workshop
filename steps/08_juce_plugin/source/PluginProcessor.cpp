#include "PluginProcessor.h"

#include <cstdio>

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
    m_load_error.clear();

    // Loading takes a second and the host may call this often — once per
    // channel is enough, and only when the channel count actually changed.
    try {
        while (m_engines.size() < channels) {
            m_engines.push_back(std::make_unique<OnnxEngine>(WORKSHOP_ONNX_MODEL_PATH));
            m_channels.push_back(std::make_unique<LatencyProcessor<OnnxEngine>>(*m_engines.back()));
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        m_load_error = error.what();
        m_engines.clear();
        m_channels.clear();
        setLatencySamples(0);
        return;
    }

    // Buffers and threads do get rebuilt: the block size may have changed.
    for (size_t channel = 0; channel < channels; ++channel) {
        m_channels[channel]->prepare(static_cast<size_t>(samples_per_block));
    }

    // The number from step 7, now where it belongs: the host delays every other
    // track by this much, so our output lines up with the rest of the session.
    setLatencySamples(static_cast<int>(m_channels.front()->latency_samples()));

    // The model was trained at one rate. Running it at another is a different
    // instrument, not a bug — worth hearing, worth knowing.
    if (std::abs(sample_rate - k_model.m_sample_rate) > 1.0) {
        m_load_error = "running at " + juce::String(sample_rate, 0) + " Hz, the model expects " +
                       juce::String(k_model.m_sample_rate, 0) + " Hz";
    }
}

void WorkshopPluginProcessor::releaseResources() {
    for (auto& channel : m_channels) { channel->stop(); }
}

// The audio callback. Everything in here comes from steps 6 and 7: no engine,
// no allocation, no lock — only the hand-over.
void WorkshopPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;

    const auto num_samples = static_cast<size_t>(buffer.getNumSamples());
    const auto channels = static_cast<size_t>(getTotalNumInputChannels());

    if (m_channels.size() < channels) {
        buffer.clear();  // The model did not load; silence beats garbage
        return;
    }

    const float mix = m_mix != nullptr ? m_mix->load() : 1.0f;

    for (size_t channel = 0; channel < channels; ++channel) {
        m_channels[channel]->set_mix(mix);
        m_channels[channel]->process_block(buffer.getWritePointer(static_cast<int>(channel)),
                                           num_samples);
    }
}

juce::AudioProcessorEditor* WorkshopPluginProcessor::createEditor() {
    return new juce::GenericAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new WorkshopPluginProcessor();
}
