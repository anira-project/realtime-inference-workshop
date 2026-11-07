// The plugin shell steps 8 to 10 share, given complete: buses, the dry/wet
// parameter, a step's own knobs, the source player, the window, and the host
// boilerplate. A step derives from it and writes the places where the host
// talks to it: prepareToPlay, processBlock, releaseResources, and the latency
// it reports.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <cstddef>
#include <vector>

#include "SourcePlayer.h"

class PluginBase : public juce::AudioProcessor {
public:
    // One of a step's own parameters, a slider in a row of its own.
    struct Knob {
        const char* m_id;  // Also the slider's label
        float m_min;
        float m_max;
        float m_default;
    };

    // @title, @subtitle: the window's header
    // @knobs: the step's own parameters, besides dry/wet
    PluginBase(juce::String title, juce::String subtitle, std::vector<Knob> knobs = {});
    ~PluginBase() override;

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

    // For the window. Message thread.
    juce::AudioProcessorValueTreeState& parameters() { return m_parameters; }
    SourcePlayer& player() { return m_player; }
    const juce::String& title() const { return m_title; }
    const juce::String& subtitle() const { return m_subtitle; }
    const std::vector<Knob>& knobs() const { return m_knobs; }
    // Why nothing plays, or nullptr while the plugin processes.
    const char* bypass_reason() const { return m_bypass.load(std::memory_order_acquire); }
    // Reads a file as your file and plays it, with the audio thread held off
    // while it changes. Returns false if it cannot be read.
    bool load_file(const juce::File& file);

protected:
    // Nothing plays: the output is silent, and the window says why. A string
    // literal, so this is safe on the audio thread too.
    void bypass(const char* reason) { m_bypass.store(reason, std::memory_order_release); }
    void process_normally() { m_bypass.store(nullptr, std::memory_order_release); }
    bool bypassed() const { return m_bypass.load(std::memory_order_acquire) != nullptr; }

    // The example or your file replaces the input; the input stays as it is.
    // While bypassed, nothing plays: the buffer is cleared and the player waits
    // at the start, so it starts over once the plugin processes again. Audio
    // thread.
    void render_source(juce::AudioBuffer<float>& buffer) {
        if (bypassed()) {
            buffer.clear();
            m_player.rewind();
            return;
        }
        m_player.render(buffer);
    }

    // The model is mono: the input channels mixed down into the first one, in
    // place. Returns it. Audio thread.
    float* to_mono(juce::AudioBuffer<float>& buffer) const;

    // The first channel copied to every output channel. Audio thread.
    void to_all_channels(juce::AudioBuffer<float>& buffer) const;

    // The dry/wet position, 0 dry to 1 wet. Audio thread.
    float mix() const { return m_mix != nullptr ? m_mix->load() : 1.0f; }

    // The value of the step's knob at `index`, in the order given. Audio thread.
    float knob(size_t index) const { return m_knob_values[index]->load(); }

private:
    juce::String m_title;
    juce::String m_subtitle;
    std::vector<Knob> m_knobs;
    juce::AudioProcessorValueTreeState m_parameters;
    std::atomic<float>* m_mix = nullptr;
    std::vector<std::atomic<float>*> m_knob_values;
    SourcePlayer m_player;
    std::atomic<const char*> m_bypass{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBase)
};
