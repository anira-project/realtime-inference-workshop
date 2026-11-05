#include "PluginProcessor.h"

#include <cmath>
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
// including loading the model and starting the worker thread.
void WorkshopPluginProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    m_bypass.store(Bypass::not_prepared, std::memory_order_release);
    m_wrong_block.store(false, std::memory_order_relaxed);
    m_host_sample_rate.store(sample_rate, std::memory_order_relaxed);
    m_host_block_size.store(samples_per_block, std::memory_order_relaxed);
    setLatencySamples(0);
    if (m_processor != nullptr) { m_processor->stop(); }

    // Loading takes a second and the host may call this often: once is
    // enough. The model is mono, so one engine serves every channel.
    try {
        if (m_engine == nullptr) {
            m_engine = std::make_unique<OnnxEngine>(k_model.m_path);
            m_processor = std::make_unique<ProcessorExample>(*m_engine);
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        m_model_error = error.what();
        m_engine.reset();
        m_processor.reset();
        m_bypass.store(Bypass::model, std::memory_order_release);
        return;
    }

    // The processor is built for one buffer size, and for the model's rate
    // or 48 kHz. Anything else needs a different design: that is part 2.
    const bool supported_rate =
        std::abs(sample_rate - 44100.0) < 1.0 || std::abs(sample_rate - 48000.0) < 1.0;
    if (!supported_rate) {
        m_bypass.store(Bypass::sample_rate, std::memory_order_release);
        return;
    }
    if (static_cast<size_t>(samples_per_block) != k_host_block_size) {
        m_bypass.store(Bypass::block_size, std::memory_order_release);
        return;
    }

    // As in step 7's main: prepare, and the worker thread starts.
    m_processor->prepare(k_host_block_size);

    // The number from step 7, now where it belongs: the host delays every other
    // track by this much, so our output lines up with the rest of the session.
    setLatencySamples(static_cast<int>(m_processor->latency_samples()));
    m_bypass.store(Bypass::none, std::memory_order_release);
}

juce::String WorkshopPluginProcessor::bypass_reason() const {
    const juce::String part_2 = "Arbitrary sample rates and buffer sizes: part 2, anira.";
    switch (m_bypass.load(std::memory_order_acquire)) {
        case Bypass::none: break;
        case Bypass::not_prepared: return {};
        case Bypass::model: return "The model did not load: " + m_model_error;
        case Bypass::sample_rate:
            return "The host runs at " +
                   juce::String(m_host_sample_rate.load(std::memory_order_relaxed), 0) +
                   " Hz. This plugin is built for 44100 or 48000 Hz. " + part_2;
        case Bypass::block_size:
            return "The host asks for buffers of " +
                   juce::String(m_host_block_size.load(std::memory_order_relaxed)) +
                   " samples. This plugin is built for 512: set the audio device or the DAW "
                   "to 512. " +
                   part_2;
    }
    if (m_wrong_block.load(std::memory_order_relaxed)) {
        return "The host sent buffers that are not 512 samples: those go out unprocessed. "
               "This plugin is built for 512 only. " +
               part_2;
    }
    return {};
}

void WorkshopPluginProcessor::releaseResources() {
    if (m_processor != nullptr) { m_processor->stop(); }
}

// The audio callback. The processing is step 7's process_block(): no engine,
// no allocation, no lock, only the hand-over.
void WorkshopPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;

    const int num_samples = buffer.getNumSamples();
    const int channels = getTotalNumInputChannels();

    // A playing example slot replaces the input, also when bypassed.
    m_player.render(buffer);

    // Bypassed: the input goes out unprocessed.
    if (m_bypass.load(std::memory_order_acquire) != Bypass::none || channels == 0 ||
        num_samples == 0) {
        return;
    }
    if (static_cast<size_t>(num_samples) != k_host_block_size) {
        m_wrong_block.store(true, std::memory_order_relaxed);  // The editor says so
        return;
    }

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
