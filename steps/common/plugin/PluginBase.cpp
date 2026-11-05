#include "PluginBase.h"

#include "PluginEditor.h"

namespace {
constexpr const char* k_mix_id = "mix";

juce::AudioProcessorValueTreeState::ParameterLayout make_layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{k_mix_id, 1},
                                                           "Dry/Wet",
                                                           0.0f,
                                                           1.0f,
                                                           1.0f));
    return layout;
}
}  // namespace

PluginBase::PluginBase(juce::String title, juce::String subtitle)
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , m_title(std::move(title))
    , m_subtitle(std::move(subtitle))
    , m_parameters(*this, nullptr, "state", make_layout()) {
    m_mix = m_parameters.getRawParameterValue(k_mix_id);
}

PluginBase::~PluginBase() = default;

bool PluginBase::load_file(const juce::File& file) {
    suspendProcessing(true);
    const bool loaded = m_player.load(file);
    suspendProcessing(false);
    if (loaded) { m_player.select(SourcePlayer::Source::file); }
    return loaded;
}

float* PluginBase::to_mono(juce::AudioBuffer<float>& buffer) const {
    const int num_samples = buffer.getNumSamples();
    const int channels = getTotalNumInputChannels();
    float* mono = buffer.getWritePointer(0);
    if (channels == 0) {
        juce::FloatVectorOperations::clear(mono, num_samples);
        return mono;
    }
    for (int channel = 1; channel < channels; ++channel) {
        juce::FloatVectorOperations::add(mono, buffer.getReadPointer(channel), num_samples);
    }
    juce::FloatVectorOperations::multiply(mono, 1.0f / static_cast<float>(channels), num_samples);
    return mono;
}

void PluginBase::to_all_channels(juce::AudioBuffer<float>& buffer) const {
    for (int channel = 1; channel < getTotalNumOutputChannels(); ++channel) {
        buffer.copyFrom(channel, 0, buffer, 0, 0, buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* PluginBase::createEditor() {
    return new PluginEditor(*this);
}
