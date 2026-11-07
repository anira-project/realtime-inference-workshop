#include "PluginProcessor.h"

#include "AniraReport.h"


PluginProcessor::PluginProcessor() : PluginBase("Workshop anira", "Step 9: the model on anira") {}

// Everything that allocates, plans or warms up belongs here. anira takes any
// sample rate and buffer size.
size_t PluginProcessor::prepare(double sample_rate, size_t max_block_size) {
    m_anira.prepare(sample_rate, max_block_size);
    m_dry_wet.prepare(max_block_size, m_anira.latency());

    // The model is trained at 44.1 kHz: any other rate waits for the user
    ask_unless_44k(sample_rate);

    // What prepare measured, for the window: not real-time holds the plugin back
    set_report({report_row("model", m_anira.plan())});

    // The host delays every other track by this much
    return m_anira.latency();
}

// The audio callback, on the mono mix-down: no allocation, no lock, no wait.
void PluginProcessor::process(float* samples, size_t num_samples) {
    m_dry_wet.store(samples, num_samples);
    // A bounce (isNonRealtime) waits for every result; live never waits
    m_anira.process(samples, num_samples, isNonRealtime());
    m_dry_wet.mix(samples, num_samples, mix());  // Dry only while held back
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return create_plugin<PluginProcessor>();
}
