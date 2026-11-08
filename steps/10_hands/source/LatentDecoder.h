// The decoder as a generator: no audio in, eight latents in, audio out. The
// latents are set from anywhere; the audio thread hands one frame of them to
// the decoder per 2048 samples it pulls out.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <memory>
#include <string>

namespace hands {

inline constexpr int k_num_latents = 8;
inline constexpr double k_model_sample_rate = 44100.0;
inline constexpr int k_model_block_size = 2048;

using Latents = std::array<float, k_num_latents>;

class LatentDecoder : public juce::AudioSource {
public:
    // Loads decoder.json and contract.json from `step_dir` and warms the model up.
    // Blocking; call off the audio thread. Throws on failure.
    explicit LatentDecoder(const std::string& step_dir);
    ~LatentDecoder() override;

    // Any thread; applies to the next frame the audio thread hands over.
    void setLatents(const Latents& latents);
    void setVolume(float gain);
    void setSounding(bool sounding);  // fades the output in or out (~1 s)

    float getOutputLevel() const { return m_output_level.load(std::memory_order_relaxed); }
    unsigned int getLatencySamples() const;  // at the model rate

    void prepareToPlay(int samples_per_block, double sample_rate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;

private:
    class Source;  // anira stays behind this file: its C++ header is not for every TU

    std::unique_ptr<Source> m_source;
    std::unique_ptr<juce::ResamplingAudioSource> m_resampler;

    std::atomic<float> m_volume{0.8F};
    std::atomic<bool> m_sounding{false};
    std::atomic<float> m_output_level{0.0F};
    juce::SmoothedValue<float> m_volume_smoothed;
    juce::SmoothedValue<float> m_sounding_smoothed;
};

}  // namespace hands
