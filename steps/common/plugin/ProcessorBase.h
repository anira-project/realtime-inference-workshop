// The plugin shell the anira steps share, taken from step 8: the dry/wet
// parameter, the example player, the window, and the host boilerplate. A step
// derives from it and writes the three places where the host talks to it:
// prepareToPlay, processBlock and the latency it reports.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <memory>

#include "ExamplePlayer.h"

class WorkshopProcessorBase : public juce::AudioProcessor {
public:
    // @title, @subtitle: the window's header
    // @extra: the step's own parameters, next to "mix"
    WorkshopProcessorBase(juce::String title,
                          juce::String subtitle,
                          juce::AudioProcessorValueTreeState::ParameterLayout extra = {});
    ~WorkshopProcessorBase() override;

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
    ExamplePlayer& player() { return m_player; }
    juce::String load_error() const { return m_load_error; }
    const juce::String& title() const { return m_title; }
    const juce::String& subtitle() const { return m_subtitle; }

    // Replaces an example slot with a file, with the audio thread held off
    // while the slot changes. Message thread only.
    bool load_example(int slot, const juce::File& file);

    // The step's own controls, below the mix, or none. Message thread.
    virtual std::unique_ptr<juce::Component> create_controls() { return nullptr; }
    virtual int controls_height() const { return 0; }

protected:
    // The dry/wet position, 0 dry to 1 wet. Audio thread.
    float mix() const { return m_mix != nullptr ? m_mix->load() : 1.0f; }

    ExamplePlayer m_player;
    juce::String m_load_error;  // What went wrong, shown in the window

private:
    juce::String m_title;
    juce::String m_subtitle;
    juce::AudioProcessorValueTreeState m_parameters;
    std::atomic<float>* m_mix = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkshopProcessorBase)
};
