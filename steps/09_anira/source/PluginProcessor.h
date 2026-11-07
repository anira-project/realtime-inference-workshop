// Step 9 — the plugin around AniraProcessor, given complete.
//
// Goal: step 8's plugin, with AniraProcessor in place of ProcessorExample. The
//       same three places: the constructor creates, prepare() plans and
//       returns the latency, process() moves one mono block. The window, the
//       source player, the bypass, the mono mix-down and the boilerplate are
//       PluginBase, shared with steps 8 and 10.
#pragma once

#include "AniraProcessor.h"
#include "DryWet.h"
#include "PluginBase.h"

class PluginProcessor final : public PluginBase {
public:
    PluginProcessor();

private:
    size_t prepare(double sample_rate, size_t max_block_size) override;
    void process(float* samples, size_t num_samples) override;

    AniraProcessor m_anira{WORKSHOP_STEP_DIR};
    DryWet m_dry_wet;  // Step 7's dry path: anira returns the wet signal only
};
