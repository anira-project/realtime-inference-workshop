#include "PluginProcessor.h"

#include "common/inference_time.h"

// Create: the processor is made, a plain member, with the model, and makes
// its engine, but nothing is loaded or allocated yet: a host may construct us
// just to scan the plugin. The model loads in prepare, with the host's settings.
PluginProcessor::PluginProcessor()
    : PluginBase("Workshop Inference", "Step 8: step 7's processor in a plugin") {}

// The host says how it will call us: load the model, size the ring buffers,
// start the worker, report the latency. The host may call this again at any time.
size_t PluginProcessor::prepare(double sample_rate, size_t host_block_size) {
    // Built for 512-sample buffers at 44.1 kHz, the only settings the prefill
    // and the latency are right for: anything else is bypassed, 48 kHz asks
    // first (PluginBase). Part 2 does the rest.
    if (!fixed_settings_fit(sample_rate, host_block_size)) { return 0; }

    // The processor loads the model the first time, it takes a second and the
    // host may call this often. A model that does not load throws, and
    // PluginBase bypasses the plugin. The block size is 512:
    // fixed_settings_fit made sure. The worst case is your step 4 number.
    m_processor.prepare(host_block_size, k_max_inference_ms);

    // The number from step 7, now where it belongs: the host delays every other
    // track by this much, so our output lines up with the rest of the session.
    return m_processor.latency_samples();
}

// The audio callback: step 7's process_block(), on the mono mix-down.
void PluginProcessor::process(float* samples, size_t num_samples) {
    m_processor.set_mix(mix());  // Dry only while held back at 48 kHz
    m_processor.process_block(samples, num_samples);
}

void PluginProcessor::release() {
    m_processor.stop();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return create_plugin<PluginProcessor>();
}
