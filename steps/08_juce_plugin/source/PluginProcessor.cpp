#include "PluginProcessor.h"

#include "PluginEditor.h"

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
    m_load_error.clear();

    // Loading takes a second and the host may call this often: once is
    // enough. The model is mono, so one engine serves every channel.
    try {
        if (m_engine == nullptr) {
            m_engine = std::make_unique<OnnxEngine>(WORKSHOP_ONNX_MODEL_PATH);
            m_processor = std::make_unique<LatencyProcessor<OnnxEngine>>(*m_engine);
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        m_load_error = error.what();
        m_engine.reset();
        m_processor.reset();
        setLatencySamples(0);
        return;
    }

    // Buffers and the thread do get rebuilt: the block size may have changed.
    m_processor->prepare(static_cast<size_t>(samples_per_block));

    // The number from step 6, now where it belongs: the host delays every other
    // track by this much, so our output lines up with the rest of the session.
    setLatencySamples(static_cast<int>(m_processor->latency_samples()));

    // The model was trained at one rate. Running it at another is a different
    // instrument, not a bug — worth hearing, worth knowing.
    if (std::abs(sample_rate - k_model.m_sample_rate) > 1.0) {
        m_load_error = "running at " + juce::String(sample_rate, 0) + " Hz, the model expects " +
                       juce::String(k_model.m_sample_rate, 0) + " Hz";
    }
}

void WorkshopPluginProcessor::releaseResources() {
    if (m_processor != nullptr) { m_processor->stop(); }
}

// The audio callback. Everything in here comes from steps 5 and 6: no engine,
// no allocation, no lock — only the hand-over.
void WorkshopPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;

    const int num_samples = buffer.getNumSamples();
    const int channels = getTotalNumInputChannels();

    if (m_processor == nullptr || channels == 0) {
        buffer.clear();  // The model did not load; silence beats garbage
        return;
    }

    // A playing example slot replaces the input.
    m_player.render(buffer);

    // The model is mono: the channels mixed down into the first one, in place
    float* mono = buffer.getWritePointer(0);
    for (int channel = 1; channel < channels; ++channel) {
        juce::FloatVectorOperations::add(mono, buffer.getReadPointer(channel), num_samples);
    }
    juce::FloatVectorOperations::multiply(mono, 1.0f / static_cast<float>(channels), num_samples);

    m_processor->set_mix(m_mix != nullptr ? m_mix->load() : 1.0f);
    m_processor->process_block(mono, static_cast<size_t>(num_samples));

    // The one result, on every output channel
    for (int channel = 1; channel < getTotalNumOutputChannels(); ++channel) {
        buffer.copyFrom(channel, 0, buffer, 0, 0, num_samples);
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
