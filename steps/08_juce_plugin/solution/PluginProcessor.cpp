#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <exception>

PluginProcessor::PluginProcessor()
    : PluginBase("Workshop Inference", "Step 8: step 7's processor in a plugin") {}

// The host says how it will call us. Everything that allocates belongs here:
// loading the model, and preparing the processor, which starts its worker.
void PluginProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    setLatencySamples(0);

    if (m_processor != nullptr) {
        m_processor->stop();
    }

    // Loading takes a second and the host may call this often: once is enough.
    if (m_engine == nullptr) {
        try {
            m_engine = std::make_unique<OnnxEngine>(k_model.m_path);
            m_processor = std::make_unique<ProcessorExample>(*m_engine);
        } catch (const std::exception& error) {
            std::fprintf(stderr, "plugin: %s\n", error.what());
            m_engine.reset();
            bypass("The model did not load, see the console.");
            return;
        }
    }

    // Built for 512-sample buffers at 44.1 kHz: the model is trained at 44.1
    // kHz, and the prefill and the latency are only right for those. At 48
    // kHz it waits for the user (k_not_44k): it plays higher, and the worker's
    // time is rounded at 44.1 kHz, so at 48 kHz the same samples last about
    // 8% less. Anything else is bypassed. Part 2 does the rest.
    const bool at_44k = std::abs(sample_rate - 44100.0) < 1.0;
    const bool at_48k = std::abs(sample_rate - 48000.0) < 1.0;
    if (!at_44k && !at_48k) {
        bypass("This plugin is built for 44.1 kHz. Arbitrary sample rates: part 2, anira.");
        return;
    }
    if (static_cast<size_t>(samples_per_block) != k_host_block_size) {
        bypass(
            "This plugin is built for buffers of 512 samples: set the audio device or the DAW "
            "to 512. Arbitrary buffer sizes: part 2, anira.");
        return;
    }

    m_processor->prepare(k_host_block_size);

    // The number from step 7, now where it belongs: the host delays every other
    // track by this much, so our output lines up with the rest of the session.
    setLatencySamples(static_cast<int>(m_processor->latency_samples()));
    set_hold(HoldKind::sample_rate, at_48k ? &k_not_44k_step8 : nullptr);
    process_normally();
}

// The audio callback: step 7's process_block(), on the mono mix-down.
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;
    render_source(buffer);

    if (bypassed()) { return; }  // Latency 0: the input goes out as it is

    // Up to the 512 prepareToPlay announced. A shorter block now and then is
    // fine: the ring buffers move sample by sample, the stream stays in place.
    float* mono = to_mono(buffer);
    // Held back at 48 kHz: the dry path only, as late as the host was told,
    // so the track stays in line with the others
    m_processor->set_mix(processing() ? mix() : 0.0f);
    m_processor->process_block(mono, static_cast<size_t>(buffer.getNumSamples()));
    to_all_channels(buffer);
}

void PluginProcessor::releaseResources() {
    if (m_processor != nullptr) { m_processor->stop(); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
