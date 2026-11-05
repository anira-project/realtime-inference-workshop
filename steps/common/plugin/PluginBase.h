// The plugin shell steps 8 and 9 share, given complete: buses, the dry/wet
// parameter, the source player, the window, and the host boilerplate. A step
// derives from it and writes the places where the host talks to it:
// prepareToPlay, processBlock, releaseResources, and the latency it reports.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

#include "SourcePlayer.h"

class PluginBase : public juce::AudioProcessor {
public:
    // @title, @subtitle: the window's header
    PluginBase(juce::String title, juce::String subtitle);
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
    // Why the input goes out unprocessed, or nullptr while the plugin processes.
    const char* bypass_reason() const { return m_bypass.load(std::memory_order_acquire); }
    // Reads a file as your file and plays it, with the audio thread held off
    // while it changes. Returns false if it cannot be read.
    bool load_file(const juce::File& file);

protected:
    // The input goes out unprocessed, and the window says why. A string
    // literal, so this is safe on the audio thread too.
    void bypass(const char* reason) { m_bypass.store(reason, std::memory_order_release); }
    void process_normally() { m_bypass.store(nullptr, std::memory_order_release); }
    bool bypassed() const { return m_bypass.load(std::memory_order_acquire) != nullptr; }

    // The example or your file replaces the input; the input stays as it is.
    // Audio thread.
    void render_source(juce::AudioBuffer<float>& buffer) { m_player.render(buffer); }

    // The model is mono: the input channels mixed down into the first one, in
    // place. Returns it. Audio thread.
    float* to_mono(juce::AudioBuffer<float>& buffer) const;

    // The first channel copied to every output channel. Audio thread.
    void to_all_channels(juce::AudioBuffer<float>& buffer) const;

    // The dry/wet position, 0 dry to 1 wet. Audio thread.
    float mix() const { return m_mix != nullptr ? m_mix->load() : 1.0f; }

private:
    juce::String m_title;
    juce::String m_subtitle;
    juce::AudioProcessorValueTreeState m_parameters;
    std::atomic<float>* m_mix = nullptr;
    SourcePlayer m_player;
    std::atomic<const char*> m_bypass{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBase)
};
