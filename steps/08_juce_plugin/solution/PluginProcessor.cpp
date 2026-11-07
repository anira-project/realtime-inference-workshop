#include "PluginProcessor.h"

// Create: the engine and the processor are made, as plain members, but they
// load and allocate nothing yet: a host may construct us just to scan the
// plugin. The model and the host's settings come in prepare.
PluginProcessor::PluginProcessor()
    : PluginBase("Workshop Inference", "Step 8: step 7's processor in a plugin") {}

// The host says how it will call us: load the model, size the ring buffers,
// start the worker, report the latency. The host may call this again at any time.
size_t PluginProcessor::prepare(double sample_rate, size_t max_block_size) {
    // Built for 512-sample buffers at 44.1 kHz, the only settings the prefill
    // and the latency are right for: anything else is bypassed, 48 kHz asks
    // first (PluginBase). Part 2 does the rest.
    if (!fixed_settings_fit(sample_rate, max_block_size)) { return 0; }

    // Loading takes a second and the host may call this often: once is enough.
    // A model that does not load throws, and PluginBase bypasses the plugin.
    if (!m_engine.loaded()) { m_engine.load(k_model.m_path); }

    m_processor.prepare(max_block_size);  // 512: fixed_settings_fit made sure

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
