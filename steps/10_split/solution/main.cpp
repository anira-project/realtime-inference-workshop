// Step 10 — Encoder, latents, decoder, reference implementation
//
// Goal:  the model cut in the middle: two handlers on one context, and the
//        eight latents per block in between, where a bias and a scale per
//        dimension are the instrument.
// Given: encoder.json, decoder.json, contract.json, the fake host.
// Check: with zero bias and unit scale the output equals the forward model's
//        reference, shifted by the latency the decoder reports.

#include <anira/anira.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/helpers/host.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

constexpr struct {
    size_t m_block_size = 512;
    double m_sample_rate = 44100.0;
} k_host{};

constexpr size_t k_num_latents = 8;
constexpr size_t k_frame_size = 2048;     // Audio samples per latent frame
constexpr size_t k_model_latency = 2224;  // Declared on the decoder's output

namespace {

// One model as one handler, over a pipeline with one inference stage.
anira::InferenceHandler create(anira::Context& context, const std::string& model_file) {
    anira::Pipeline pipeline;
    pipeline.add_inference(anira::ModelConfig::from_file(model_file));
    return anira::InferenceHandler(context, pipeline);
}

// The contract with the host's blocks, prepared; returns the latency of output slot 0.
size_t prepare(anira::InferenceHandler& handler, anira::ContractHandle& contract) {
    contract.hard_host_blocks(k_host.m_block_size, k_host.m_block_size, k_host.m_sample_rate);
    handler.prepare(contract);
    return handler.get_latency(0);
}

}  // namespace

int main() try {
    const std::string step_dir = WORKSHOP_STEP_DIR;

    // The controls: set from anywhere, read on the audio thread.
    std::array<std::atomic<float>, k_num_latents> bias;
    std::array<std::atomic<float>, k_num_latents> scale;
    for (size_t i = 0; i < k_num_latents; ++i) {
        bias[i].store(0.0f);
        scale[i].store(1.0f);
    }

    anira::Context context{anira::ContextConfig{}};
    anira::ContractHandle contract = anira::ContractHandle::from_file(step_dir + "/contract.json");

    // ---- TODO 1: the encoder -----------------------------------------------
    anira::InferenceHandler encoder = create(context, step_dir + "/encoder.json");
    const size_t encoder_latency_frames = prepare(encoder, contract);  // In latent frames: its output's unit

    // ---- TODO 3: the decoder -----------------------------------------------
    anira::InferenceHandler decoder = create(context, step_dir + "/decoder.json");
    const size_t decoder_latency = prepare(decoder, contract);  // In samples, on its own

    // The decoder's frames arrive late by the encoder's latency. Declared on
    // the decoder's output, the sum is what the host is told, and what the
    // decoder's ring is primed to: the frame has time to arrive.
    const size_t latency = encoder_latency_frames * k_frame_size + decoder_latency;
    contract.hard_latency("audio", static_cast<uint32_t>(latency));
    if (prepare(decoder, contract) != latency) { throw std::runtime_error("the declared latency was not taken"); }
    // ------------------------------------------------------------------------

    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                latency,
                1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                k_host.m_sample_rate);

    // One latent frame [8, 1], its tensor built once
    std::array<float, k_num_latents> frame_data{};
    const int64_t frame_shape[2] = {k_num_latents, 1};
    const anira::Tensor frame = anira::Tensor::from_host(frame_data.data(), ANIRA_DTYPE_F32, frame_shape);

    std::vector<size_t> late;
    size_t position = 0;
    size_t frames = 0;

    const std::vector<float> output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host.m_block_size,
        [&](float* samples, size_t num_samples) {
            // One tensor over the host's block, [channels, samples]: no copy, no allocation
            const int64_t block_shape[2] = {1, static_cast<int64_t>(num_samples)};
            const anira::Tensor block = anira::Tensor::from_host(samples, ANIRA_DTYPE_F32, block_shape);

            // ---- TODO 1: the encoder ---------------------------------------
            // Audio in. A latent frame comes out once a whole block has been
            // encoded, about one latency later; usually none in this callback.
            // Pop only what is there: a pop with no frame waiting is a miss.
            encoder.push_data(block, 0);
            size_t available = 0;
            while (encoder.get_available_samples(0, 0, &available) == ANIRA_OK && available > 0) {
                encoder.pop_data(frame, 0);
                // The encoder's first frames are its latency priming: zeros. A
                // zero latent is not silence, it is a sound of its own, so
                // those frames must not reach the decoder.
                const bool is_priming = frames < encoder_latency_frames;
                ++frames;
                if (is_priming) { continue; }

                // ---- TODO 2: bias and scale on z ---------------------------
                for (size_t i = 0; i < k_num_latents; ++i) {
                    frame_data[i] = frame_data[i] * scale[i].load() + bias[i].load();
                }

                // ---- TODO 3: the decoder -----------------------------------
                decoder.push_data(frame, 0);
            }
            // The decoder's audio out, for this callback: samples, or the miss policy
            const anira_status status = decoder.pop_data(block, 0);
            if (status == ANIRA_MISSED) { late.push_back(position); }
            // ----------------------------------------------------------------
            position += num_samples;
        },
        k_host.m_sample_rate);

    std::printf("  latent frames                %zu, %zu of them priming\n", frames, encoder_latency_frames);

    // The reference carries the model's own delay, which the decoder declared:
    // it lines up after the rest, as in steps 6 and 9.
    return report_with_latency(output.data(), k_target_output_signal.data(), output.size(),
                               latency - k_model_latency, late, k_host.m_block_size);
} catch (const std::exception& error) {
    // Every refusal of anira's, from the files to prepare, names what it refused
    std::fprintf(stderr, "%s\n", error.what());
    return 2;
}
