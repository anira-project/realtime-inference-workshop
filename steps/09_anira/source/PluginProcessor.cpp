#include "PluginProcessor.h"

#include <cstdio>
#include <exception>

PluginProcessor::PluginProcessor() : PluginBase("Workshop anira", "Step 9: the model on anira") {
    try {
        m_anira = std::make_unique<AniraProcessor>(WORKSHOP_STEP_DIR);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        bypass("The model did not load, see the console.");
    }
}

// Everything that allocates, plans or warms up belongs here. anira takes any
// sample rate and buffer size.
void PluginProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    setLatencySamples(0);
    if (m_anira == nullptr) { return; }  // Bypassed since the constructor

    try {
        m_anira->prepare(sample_rate, static_cast<size_t>(samples_per_block));
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        bypass("anira could not prepare the model, see the console.");
        return;
    }
    m_dry_wet.prepare(getTotalNumInputChannels(), samples_per_block, m_anira->latency());

    // The host delays every other track by this much
    setLatencySamples(static_cast<int>(m_anira->latency()));
    process_normally();
}

// The audio callback: no allocation, no lock, no wait.
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;
    render_source(buffer);

    if (bypassed()) { return; }

    m_dry_wet.store(buffer);
    float* mono = to_mono(buffer);
    // A bounce (isNonRealtime) waits for every result; live never waits
    m_anira->process(mono, static_cast<size_t>(buffer.getNumSamples()), isNonRealtime());
    to_all_channels(buffer);
    m_dry_wet.mix(buffer, mix());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
