// Step 8 — The plugin, given complete.
//
// Goal: everything from steps 1 to 7 inside a JUCE plugin. The only new parts
//       are the three places where the host talks to us: prepareToPlay,
//       processBlock, and the latency we report.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

#include "common/helpers/onnx_engine.h"
#include "common/helpers/threaded_processor.h"

class WorkshopPluginProcessor : public juce::AudioProcessor {
public:
    WorkshopPluginProcessor();
    ~WorkshopPluginProcessor() override;

    void prepareToPlay(double sample_rate, int samples_per_block) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::AudioProcessorValueTreeState& parameters() { return m_parameters; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout make_layout();

    juce::AudioProcessorValueTreeState m_parameters;
    std::atomic<float>* m_mix = nullptr;

    // ONNX Runtime, not LibTorch: it is the one with a static build, so the
    // plugin is a single self-contained bundle — step 4, made good on.
    // One engine and one processor per channel: the model is mono, and its
    // state belongs to the signal that is running through it.
    std::vector<std::unique_ptr<OnnxEngine>> m_engines;
    std::vector<std::unique_ptr<LatencyProcessor<OnnxEngine>>> m_channels;
    juce::String m_load_error;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkshopPluginProcessor)
};
