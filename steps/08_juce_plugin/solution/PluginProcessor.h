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
#include "common/helpers/onnx_engine.h"

class PluginProcessor final : public PluginBase {
public:
    PluginProcessor();

private:
    size_t prepare(double sample_rate, size_t max_block_size) override;
    void process(float* samples, size_t num_samples) override;
    void release() override;

    // One engine and one processor, as in step 7's main. The processor runs
    // its own worker thread.
    OnnxEngine m_engine{k_model.m_path};
    ProcessorExample m_processor{m_engine};
};
