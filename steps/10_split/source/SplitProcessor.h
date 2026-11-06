// Step 10 — Encoder, latents, decoder: the processor.
//
// The model cut in the middle, as in anira's encoder/decoder example
// (examples/juce-audio-plugin, model 7): two handlers on one context, and one
// latent frame of eight between them per k_frame_size input samples, where an
// offset per dimension is the instrument. The plugin (source/PluginProcessor.cpp)
// and the check (check.cpp) both drive it the way a host does: prepare once
// before the stream, process in every callback.
#pragma once

#include <anira/anira.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <string>

class SplitProcessor {
public:
    static constexpr size_t k_num_latents = 8;
    static constexpr size_t k_frame_size = 2048;  // Audio samples per latent frame

    // Reads encoder.json, decoder.json and contract.json from `step_dir`. Main thread.
    // @throws anira::Error with anira's reason
    explicit SplitProcessor(const std::string& step_dir);

    // Before the stream starts: the host's rate and its largest block. Plans
    // both handlers and warms the models up. Main thread, blocking.
    // @throws anira::Error with anira's reason
    void prepare(double sample_rate, size_t max_block_size);

    // One callback, in place: one mono block in, the decoder's audio out.
    // Audio thread: no allocation, no lock, no wait.
    // @return true if the miss policy filled the block
    bool process(float* samples, size_t num_samples);

    // The knob of one latent dimension. Any thread.
    void set_offset(size_t dimension, float value) { m_offset[dimension].store(value); }

    // What the host is told, in samples. Valid after prepare.
    size_t latency() const { return m_latency; }
    // The budgets prepare measured on this machine, in ms. Valid after prepare.
    double encoder_budget_ms() const { return m_encoder_budget_ms; }
    double decoder_budget_ms() const { return m_decoder_budget_ms; }
    // The encoder's latency, in latent frames. Valid after prepare.
    size_t encoder_latency_frames() const { return m_encoder_latency_frames; }
    // Latent frames popped from the encoder since prepare.
    size_t frames() const { return m_frames; }

private:
    // In this order: the handlers run on the context and go before it
    anira::Context m_context{anira::ContextConfig{}};
    anira::ModelConfig m_encoder_model;
    anira::ModelConfig m_decoder_model;
    anira::ContractHandle m_contract;
    std::optional<anira::InferenceHandler> m_encoder;  // Created at the first prepare
    std::optional<anira::InferenceHandler> m_decoder;

    // One latent frame [8, 1], its tensor built once over the array
    std::array<float, k_num_latents> m_frame_data{};
    anira::Tensor m_frame;

    std::array<std::atomic<float>, k_num_latents> m_offset{};
    size_t m_input_samples = 0;  // Towards the next latent frame
    size_t m_frames = 0;
    size_t m_encoder_latency_frames = 0;
    size_t m_latency = 0;
    double m_encoder_budget_ms = 0.0;
    double m_decoder_budget_ms = 0.0;
};
