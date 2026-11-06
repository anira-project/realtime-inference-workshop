#include "PluginProcessor.h"

#include <array>
#include <cstdio>
#include <exception>
#include <vector>

namespace {
// One knob per latent dimension, -3 to 3 around 0: the range the latents move in
std::vector<PluginBase::Knob> latent_knobs() {
    static constexpr const char* k_ids[SplitProcessor::k_num_latents] = {
        "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7"};
    std::vector<PluginBase::Knob> knobs;
    for (const char* id : k_ids) { knobs.push_back({id, -3.0f, 3.0f, 0.0f}); }
    return knobs;
}
}  // namespace

PluginProcessor::PluginProcessor()
    : PluginBase("Workshop split", "Step 10: opening the model", latent_knobs()) {
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

    // The knobs, z0 to z7: one offset per latent
    std::array<float, SplitProcessor::k_num_latents> offsets{};
    for (size_t i = 0; i < offsets.size(); ++i) { offsets[i] = knob(i); }

    m_dry_wet.store(buffer);
    float* mono = to_mono(buffer);
    // A bounce (isNonRealtime) waits for every result; live never waits
    m_split->process(mono, static_cast<size_t>(buffer.getNumSamples()), offsets, isNonRealtime());
    to_all_channels(buffer);
    m_dry_wet.mix(buffer, mix());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
