// Step 9 — the plugin around AniraProcessor, given. The host's three places:
// prepareToPlay plans, processBlock moves one block, the latency is reported.
// The window, the example player and the dry/wet mix are the shared shell.

#include <cmath>
#include <cstdio>
#include <exception>
#include <memory>

#include "AniraProcessor.h"
#include "DryWet.h"
#include "ProcessorBase.h"

namespace {
constexpr double k_model_sample_rate = 44100.0;  // What the model was trained at
}

class AniraPlugin final : public WorkshopProcessorBase {
public:
    AniraPlugin() : WorkshopProcessorBase("Workshop anira", "Step 9: the model on anira") {
        try {
            m_anira = std::make_unique<AniraProcessor>(WORKSHOP_STEP_DIR);
        } catch (const std::exception& error) {
            std::fprintf(stderr, "plugin: %s\n", error.what());
            m_load_error = error.what();
        }
    }

    // Everything that allocates, plans or warms up belongs here.
    void prepareToPlay(double sample_rate, int samples_per_block) override {
        m_ready = false;
        setLatencySamples(0);
        if (m_anira == nullptr) { return; }  // The files did not load: the error is shown
        m_load_error.clear();

        const int channels = getTotalNumInputChannels();
        try {
            m_anira->prepare(sample_rate, static_cast<size_t>(samples_per_block));
        } catch (const std::exception& error) {
            std::fprintf(stderr, "plugin: %s\n", error.what());
            m_load_error = error.what();
            return;
        }
        m_dry_wet.prepare(channels, samples_per_block, m_anira->latency());

        // The host delays every other track by this much
        setLatencySamples(static_cast<int>(m_anira->latency()));
        m_ready = true;

        if (std::abs(sample_rate - k_model_sample_rate) > 1.0) {
            m_load_error = "running at " + juce::String(sample_rate, 0) + " Hz, the model expects " +
                           juce::String(k_model_sample_rate, 0) + " Hz";
        }
    }

    void releaseResources() override {}

    // The audio callback: no allocation, no lock, no wait.
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals no_denormals;
        if (!m_ready || getTotalNumInputChannels() == 0) {
            buffer.clear();  // Not prepared: silence beats garbage
            return;
        }

        m_player.render(buffer);  // A playing example slot replaces the input
        m_dry_wet.store(buffer);

        // The model is mono: the channels mixed down into the first one, in place
        const int num_samples = buffer.getNumSamples();
        const int channels = getTotalNumInputChannels();
        float* mono = buffer.getWritePointer(0);
        for (int channel = 1; channel < channels; ++channel) {
            juce::FloatVectorOperations::add(mono, buffer.getReadPointer(channel), num_samples);
        }
        juce::FloatVectorOperations::multiply(mono, 1.0f / static_cast<float>(channels), num_samples);

        m_anira->process(mono, static_cast<size_t>(num_samples));

        // The one result, on every output channel, then the dry signal mixed in
        for (int channel = 1; channel < getTotalNumOutputChannels(); ++channel) {
            buffer.copyFrom(channel, 0, buffer, 0, 0, num_samples);
        }
        m_dry_wet.mix(buffer, mix());
    }

private:
    std::unique_ptr<AniraProcessor> m_anira;
    DryWet m_dry_wet;
    bool m_ready = false;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AniraPlugin();
}
