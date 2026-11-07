// Step 8 — The plugin, given complete.
//
// Goal: step 7's ProcessorExample inside a JUCE plugin. The only new parts are
//       the three places where the host talks to us: the constructor
//       creates, prepare() prepares and returns the latency, process() runs
//       one block. The window, the source player, the bypass, the mono
//       mix-down and the boilerplate are PluginBase, shared with steps 9 and 10.
#pragma once

#include "PluginBase.h"
#include "ProcessorExample.h"

class PluginProcessor final : public PluginBase {
public:
    PluginProcessor();

private:
    size_t prepare(double sample_rate, size_t host_block_size) override;
    void process(float* samples, size_t num_samples) override;
    void release() override;

    // One processor, as in step 7's main, made with the plugin and the model.
    // It makes its engine, but neither loads nor allocates anything before
    // prepare. It runs its own worker thread.
    ProcessorExample m_processor{k_model};
};
