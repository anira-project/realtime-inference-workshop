// Step 9 — the plugin around AniraProcessor, given complete.
//
// Goal: step 8's plugin, with AniraProcessor in place of ProcessorExample. The
//       same places where the host talks to us: prepareToPlay plans,
//       processBlock moves one block, the latency is reported. The window, the
//       source player and the boilerplate are PluginBase, shared with step 8.
#pragma once

#include <memory>

#include "AniraProcessor.h"
#include "DryWet.h"
#include "PluginBase.h"

class PluginProcessor final : public PluginBase {
public:
    PluginProcessor();

    void prepareToPlay(double sample_rate, int samples_per_block) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
    void releaseResources() override {}

private:
    std::unique_ptr<AniraProcessor> m_anira;
    DryWet m_dry_wet;  // Step 7's dry path: anira returns the wet signal only
};
