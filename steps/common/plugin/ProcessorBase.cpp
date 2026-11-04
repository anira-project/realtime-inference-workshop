#include "ProcessorBase.h"

#include "PluginEditor.h"

namespace {
constexpr const char* k_mix_id = "mix";

juce::AudioProcessorValueTreeState::ParameterLayout with_mix(
    juce::AudioProcessorValueTreeState::ParameterLayout layout) {
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{k_mix_id, 1},
                                                           "Dry/Wet",
                                                           0.0f,
                                                           1.0f,
                                                           1.0f));
    return layout;
}
}  // namespace

WorkshopProcessorBase::WorkshopProcessorBase(juce::String title,
                                             juce::String subtitle,
                                             juce::AudioProcessorValueTreeState::ParameterLayout extra)
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , m_title(std::move(title))
    , m_subtitle(std::move(subtitle))
    , m_parameters(*this, nullptr, "state", with_mix(std::move(extra))) {
    m_mix = m_parameters.getRawParameterValue(k_mix_id);
}

WorkshopProcessorBase::~WorkshopProcessorBase() = default;

bool WorkshopProcessorBase::load_example(int slot, const juce::File& file) {
    suspendProcessing(true);
    const bool loaded = m_player.load(slot, file);
    suspendProcessing(false);
    return loaded;
}

juce::AudioProcessorEditor* WorkshopProcessorBase::createEditor() {
    return new WorkshopPluginEditor(*this);
}
