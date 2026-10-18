#include "LatentDecoder.h"

#include <anira/anira.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace hands {

namespace {

std::string describe(const char* call, anira_status status, const anira_error& error) {
    return std::string(call) + ": " + anira_status_string(status) + ": " + error.message;
}

// How many frames the audio thread keeps ahead of what it has pulled: one for
// the inference in flight, one queued behind it.
constexpr int k_frames_ahead = 2;

}  // namespace

// Pulls the decoder at the model rate. Requests larger than the prepared block are split.
class LatentDecoder::Source : public juce::AudioSource {
public:
    explicit Source(const std::string& step_dir)
        : m_context(anira::ContextConfig{}) {
        const anira::ModelConfig model = anira::ModelConfig::from_file(step_dir + "/decoder.json");
        m_contract = std::make_unique<anira::ContractHandle>(anira::ContractHandle::from_file(step_dir + "/contract.json"));
        const anira::Pipeline pipeline{anira::stage::Inference(model)};
        anira_error error{};
        const anira_status status = anira_handler_create(m_context.native(), pipeline.native(), &m_handler, &error);
        if (status != ANIRA_OK) { throw std::runtime_error(describe("create", status, error)); }

        const int64_t shape[2] = {k_num_latents, 1};
        anira_tensor_init_host(&m_frame, m_frame_data.data(), ANIRA_DTYPE_F32, 2, shape);
        for (size_t i = 0; i < k_num_latents; ++i) { m_latents[i].store(0.0F); }
    }

    ~Source() override { anira_handler_destroy(m_handler); }

    void setLatents(const Latents& latents) {
        for (size_t i = 0; i < latents.size(); ++i) { m_latents[i].store(latents[i], std::memory_order_relaxed); }
    }

    unsigned int getLatency() const { return m_latency; }

    void prepareToPlay(int samples_per_block, double) override {
        m_max_block = std::max(samples_per_block, 1);
        // Any block up to the resampler's largest request, at the model's rate
        m_contract->hard_geometry(1, static_cast<uint32_t>(m_max_block), k_model_sample_rate);
        anira_error error{};
        const anira_status status = anira_handler_prepare(m_handler, m_contract->native(), &error);
        if (status != ANIRA_OK) { throw std::runtime_error(describe("prepare", status, error)); }
        m_latency = anira_handler_get_latency(m_handler, 0);

        m_pulled = 0;
        m_pushed = 0;
        feed();  // The first frames, so the decoder has something to start on
    }

    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
        float* out = info.buffer->getWritePointer(0, info.startSample);
        int64_t shape[2] = {1, 0};
        for (int done = 0; done < info.numSamples;) {
            const int n = std::min(info.numSamples - done, m_max_block);
            shape[1] = n;
            anira_tensor block{};
            anira_tensor_init_host(&block, out + done, ANIRA_DTYPE_F32, 2, shape);
            size_t delivered = 0;
            anira_handler_pop_data(m_handler, &block, 0, &delivered);  // ANIRA_MISSED: the policy's zeros
            m_pulled += static_cast<size_t>(n);
            done += n;
        }
        feed();
    }

private:
    // One latent frame per model block pulled, kept k_frames_ahead in front.
    void feed() {
        while (m_pushed * k_model_block_size < m_pulled + k_frames_ahead * k_model_block_size) {
            for (size_t i = 0; i < k_num_latents; ++i) { m_frame_data[i] = m_latents[i].load(std::memory_order_relaxed); }
            anira_handler_push_data(m_handler, &m_frame, 0);
            ++m_pushed;
        }
    }

    anira::Context m_context;
    std::unique_ptr<anira::ContractHandle> m_contract;
    anira_handler* m_handler = nullptr;
    unsigned int m_latency = 0;
    int m_max_block = 512;

    std::array<std::atomic<float>, k_num_latents> m_latents;
    std::array<float, k_num_latents> m_frame_data{};
    anira_tensor m_frame{};
    size_t m_pulled = 0;  // Samples pulled out, audio thread
    size_t m_pushed = 0;  // Frames pushed in, audio thread
};

LatentDecoder::LatentDecoder(const std::string& step_dir)
    : m_source(std::make_unique<Source>(step_dir))
    , m_resampler(std::make_unique<juce::ResamplingAudioSource>(m_source.get(), false, 1)) {}

LatentDecoder::~LatentDecoder() = default;

void LatentDecoder::setLatents(const Latents& latents) {
    m_source->setLatents(latents);
}

void LatentDecoder::setVolume(float gain) {
    m_volume.store(gain, std::memory_order_relaxed);
}

void LatentDecoder::setSounding(bool sounding) {
    m_sounding.store(sounding, std::memory_order_relaxed);
}

unsigned int LatentDecoder::getLatencySamples() const {
    return m_source->getLatency();
}

void LatentDecoder::prepareToPlay(int samples_per_block, double sample_rate) {
    m_resampler->setResamplingRatio(k_model_sample_rate / sample_rate);
    m_resampler->prepareToPlay(samples_per_block, sample_rate);

    m_volume_smoothed.reset(sample_rate, 0.05);
    m_volume_smoothed.setCurrentAndTargetValue(m_volume.load(std::memory_order_relaxed));
    m_sounding_smoothed.reset(sample_rate, 1.0);
    m_sounding_smoothed.setCurrentAndTargetValue(0.0F);
}

void LatentDecoder::releaseResources() {
    m_resampler->releaseResources();
}

void LatentDecoder::getNextAudioBlock(const juce::AudioSourceChannelInfo& info) {
    m_resampler->getNextAudioBlock(info);

    m_volume_smoothed.setTargetValue(m_volume.load(std::memory_order_relaxed));
    m_sounding_smoothed.setTargetValue(m_sounding.load(std::memory_order_relaxed) ? 1.0F : 0.0F);

    float* mono = info.buffer->getWritePointer(0, info.startSample);
    float sum_squares = 0.0F;
    for (int i = 0; i < info.numSamples; ++i) {
        const float gain = m_volume_smoothed.getNextValue() * m_sounding_smoothed.getNextValue();
        const float sample = std::clamp(mono[i] * gain, -1.0F, 1.0F);  // Safety clip
        mono[i] = sample;
        sum_squares += sample * sample;
    }
    for (int channel = 1; channel < info.buffer->getNumChannels(); ++channel) {
        info.buffer->copyFrom(channel, info.startSample, *info.buffer, 0, info.startSample, info.numSamples);
    }

    const float rms = info.numSamples > 0 ? std::sqrt(sum_squares / static_cast<float>(info.numSamples)) : 0.0F;
    m_output_level.store(rms, std::memory_order_relaxed);
}

}  // namespace hands
