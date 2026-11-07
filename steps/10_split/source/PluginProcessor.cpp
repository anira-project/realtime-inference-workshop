#include "PluginProcessor.h"

#include "AniraReport.h"

#include <array>
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
    : PluginBase("Workshop split", "Step 10: opening the model", latent_knobs()) {}

// Everything that allocates, plans or warms up belongs here. anira takes any
// sample rate and buffer size.
size_t PluginProcessor::prepare(double sample_rate, size_t max_block_size) {
    m_split.prepare(sample_rate, max_block_size);
    m_dry_wet.prepare(max_block_size, m_split.latency());

    // The model is trained at 44.1 kHz: any other rate waits for the user
    ask_unless_44k(sample_rate);

    // What prepare measured, for the window: not real-time holds the plugin back
    set_report({report_row("encoder", m_split.encoder_plan()),
                report_row("decoder", m_split.decoder_plan())});

    // The host delays every other track by this much
    return m_split.latency();
}

// The audio callback, on the mono mix-down: no allocation, no lock, no wait.
void PluginProcessor::process(float* samples, size_t num_samples) {
    // The knobs, z0 to z7: one offset per latent
    std::array<float, SplitProcessor::k_num_latents> offsets{};
    for (size_t i = 0; i < offsets.size(); ++i) { offsets[i] = knob(i); }

    m_dry_wet.store(samples, num_samples);
    // Held back, the model does not run: only the dry signal goes out
    if (processing()) {
        // A bounce (isNonRealtime) waits for every result; live never waits
        m_split.process(samples, num_samples, offsets, isNonRealtime());
    }
    m_dry_wet.mix(samples, num_samples, mix());  // 0 while held back
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return create_plugin<PluginProcessor>();
}
