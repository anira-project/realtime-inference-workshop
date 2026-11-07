// Step 10 — the plugin around SplitProcessor, given complete.
//
// Goal: step 9's plugin, with SplitProcessor in place of AniraProcessor, and
//       eight knobs, z0 to z7, one offset per latent dimension, as in anira's
//       encoder/decoder example. The window, the source player and the
//       boilerplate are PluginBase, shared with steps 8 and 9.
#pragma once

#include "DryWet.h"
#include "PluginBase.h"
#include "SplitProcessor.h"

class PluginProcessor final : public PluginBase {
public:
    PluginProcessor();

private:
    size_t prepare(double sample_rate, size_t max_block_size) override;
    void process(float* samples, size_t num_samples) override;

    SplitProcessor m_split{WORKSHOP_STEP_DIR};
    DryWet m_dry_wet;  // Step 7's dry path: the decoder returns the wet signal only
};
