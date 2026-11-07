#include "PluginProcessor.h"

#include "AniraReport.h"

#include <algorithm>
#include <cmath>
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
        set_report({});  // Nothing runs: no figures to show
        bypass("anira could not prepare the model, see the console.");
        return;
    }
    // Every channel of the buffer: a mono input can feed a stereo output
    m_dry_wet.prepare(std::max(getTotalNumInputChannels(), getTotalNumOutputChannels()),
                      samples_per_block,
                      m_anira->latency());

    // The model is trained at 44.1 kHz: any other rate waits for the user
    set_hold(HoldKind::sample_rate, std::abs(sample_rate - 44100.0) < 1.0 ? nullptr : &k_not_44k);

    // What prepare measured, for the window: not real-time holds the plugin back
    set_report({report_row("model", m_anira->plan())});

    // The host delays every other track by this much
    setLatencySamples(static_cast<int>(m_anira->latency()));
    process_normally();
}

// The audio callback: no allocation, no lock, no wait.
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;
    render_source(buffer);

    if (bypassed()) { return; }  // Latency 0: the input goes out as it is
    if (!processing()) {
        // Held back, not real-time: the input delayed by the latency the host
        // was told, so the track stays in line with the others
        m_dry_wet.store(buffer);
        m_dry_wet.mix(buffer, 0.0f);
        return;
    }

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
