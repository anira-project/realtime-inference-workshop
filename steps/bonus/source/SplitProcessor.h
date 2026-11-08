// Bonus — Opening the model: the processor.
//
// The model cut in the middle, as in anira's encoder/decoder example
// (examples/juce-audio-plugin, model 7): two handlers on one context, and one
// latent frame of eight numbers between them per k_frame_size input samples.
// The plugin (source/PluginProcessor.cpp) and the check (check.cpp) both drive
// it the way a host does: prepare once before the stream, process in every
// callback.
#pragma once

#include <anira/anira.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <string>

class SplitProcessor {
public:
    static constexpr size_t k_num_latents = 8;
    static constexpr size_t k_frame_size = 2048;  // Audio samples per latent frame

    // Reads encoder.json, decoder.json and contract.json from `step_dir` and
    // creates both handlers. Main thread.
    // @throws std::runtime_error or anira::Error with the reason
    explicit SplitProcessor(const std::string& step_dir);

    // Before the stream starts: the host's rate and its largest block. Plans
    // both handlers and warms the models up. Main thread, blocking.
    // @throws anira::Error with anira's reason
    void prepare(double sample_rate, size_t max_block_size);

    // One callback, in place: one mono block in, the decoder's audio out.
    // @offsets: one per latent dimension, 0 = unchanged; the plugin reads its
    //           eight knobs into them
    // @offline: the host renders offline, a bounce: wait for every result.
    //           Live (false): audio thread, no allocation, no lock, no wait.
    // @return anira's status: ANIRA_OK, or ANIRA_MISSED if the miss policy
    // filled the block
    anira_status process(float* samples,
                         size_t num_samples,
                         const std::array<float, k_num_latents>& offsets,
                         bool offline = false);

    // What the host is told, in samples. Valid after prepare.
    size_t latency() const { return m_latency; }
    // The encoder's latency, in latent frames. Valid after prepare.
    size_t encoder_latency_frames() const { return m_encoder_latency_frames; }
    // Latent frames popped from the encoder since prepare.
    size_t frames() const { return m_frames; }
    // The encoder's and the decoder's row of their handler's report from the
    // last prepare: budget, measured and median time, warm-up, real-time
    // factor and verdict. Empty before a prepare. Their strings live until the
    // next prepare.
    anira_plan_info encoder_plan() const { return first_plan(m_encoder->plan_report()); }
    anira_plan_info decoder_plan() const { return first_plan(m_decoder->plan_report()); }

private:
    // In this order: the handlers run on the context and go before it
    anira::Context m_context{anira::ContextConfig{}};
    anira::ContractHandle m_contract;
    std::optional<anira::InferenceHandler> m_encoder;  // Created in the constructor
    std::optional<anira::InferenceHandler> m_decoder;

    // One latent frame [8, 1], its tensor built once over the array
    std::array<float, k_num_latents> m_frame_data{};
    anira::Tensor m_frame;

    size_t m_input_samples = 0;  // Towards the next latent frame
    size_t m_frames = 0;
    size_t m_encoder_latency_frames = 0;
    size_t m_latency = 0;

    static anira_plan_info first_plan(const anira::PlanReport& report) {
        return report.num_plans() > 0 ? report.plans().front() : anira_plan_info{};
    }
};
