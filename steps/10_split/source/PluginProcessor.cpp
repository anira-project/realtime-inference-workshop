#include "PluginProcessor.h"

#include <cstdio>
#include <exception>
#include <vector>

namespace {
// One knob per latent dimension, -1 to 1 around 0, as in anira's example
std::vector<PluginBase::Knob> latent_knobs() {
    static constexpr const char* k_ids[SplitProcessor::k_num_latents] = {
        "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7"};
    std::vector<PluginBase::Knob> knobs;
    for (const char* id : k_ids) { knobs.push_back({id, -1.0f, 1.0f, 0.0f}); }
    return knobs;
}
}  // namespace

PluginProcessor::PluginProcessor()
    : PluginBase("Workshop split", "Step 10: encoder, latents, decoder", latent_knobs()) {
    try {
        m_split = std::make_unique<SplitProcessor>(WORKSHOP_STEP_DIR);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        bypass("The models did not load, see the console.");
    }
}

// Everything that allocates, plans or warms up belongs here. anira takes any
// sample rate and buffer size.
void PluginProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    setLatencySamples(0);
    if (m_split == nullptr) { return; }  // Bypassed since the constructor

    try {
        m_split->prepare(sample_rate, static_cast<size_t>(samples_per_block));
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin: %s\n", error.what());
        bypass("anira could not prepare the models, see the console.");
        return;
    }
    m_dry_wet.prepare(getTotalNumInputChannels(), samples_per_block, m_split->latency());

    // The host delays every other track by this much
    setLatencySamples(static_cast<int>(m_split->latency()));
    process_normally();
}

// The audio callback: no allocation, no lock, no wait.
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;
    render_source(buffer);

    if (bypassed()) { return; }

    // The knobs, z0 to z7, to the latents
    for (size_t i = 0; i < SplitProcessor::k_num_latents; ++i) { m_split->set_offset(i, knob(i)); }

    m_dry_wet.store(buffer);
    float* mono = to_mono(buffer);
    m_split->process(mono, static_cast<size_t>(buffer.getNumSamples()));
    to_all_channels(buffer);
    m_dry_wet.mix(buffer, mix());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
