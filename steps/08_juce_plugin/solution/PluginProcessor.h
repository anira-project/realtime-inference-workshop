// Step 8 — The plugin, given complete.
//
// Goal: step 7's ProcessorExample inside a JUCE plugin. The only new parts are
//       the places where the host talks to us: prepareToPlay, processBlock,
//       releaseResources, and the latency we report. The window, the source
//       player and the boilerplate are PluginBase, shared with step 9.
#pragma once

#include <memory>

#include "PluginBase.h"
#include "ProcessorExample.h"
#include "common/helpers/onnx_engine.h"

class PluginProcessor final : public PluginBase {
public:
    PluginProcessor();

    void prepareToPlay(double sample_rate, int samples_per_block) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
    void releaseResources() override;

private:
    // One engine and one processor, as in step 7's main. The processor runs
    // its own worker thread.
    std::unique_ptr<OnnxEngine> m_engine;
    std::unique_ptr<ProcessorExample> m_processor;
};
