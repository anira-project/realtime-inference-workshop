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

struct Handler {
    anira_handler* m_handle = nullptr;
    ~Handler() { anira_handler_destroy(m_handle); }
};

std::string describe(const char* call, anira_status status, const anira_error& error) {
    return std::string(call) + ": " + anira_status_string(status) + ": " + error.message;
}

// One model as one handler, created over a pipeline with one inference stage.
void create(Handler& handler, anira::Context& context, const std::string& model_file) {
    const anira::ModelConfig model = anira::ModelConfig::from_file(model_file);
    const anira::Pipeline pipeline{anira::stage::Inference(model)};
    anira_error error{};
    const anira_status status = anira_handler_create(context.native(), pipeline.native(), &handler.m_handle, &error);
    if (status != ANIRA_OK) { throw std::runtime_error(describe("create", status, error)); }
}

// The contract with the host's geometry, prepared; returns the latency of output slot 0.
size_t prepare(Handler& handler, anira::ContractHandle& contract) {
    contract.hard_geometry(k_host.m_block_size, k_host.m_block_size, k_host.m_sample_rate);
    anira_error error{};
    const anira_status status = anira_handler_prepare(handler.m_handle, contract.native(), &error);
    if (status != ANIRA_OK) { throw std::runtime_error(describe("prepare", status, error)); }
    return anira_handler_get_latency(handler.m_handle, 0);
}

}  // namespace

int main() {
    const std::string step_dir = WORKSHOP_STEP_DIR;

    // The controls: set from anywhere, read on the audio thread.
    std::array<std::atomic<float>, k_num_latents> bias;
    std::array<std::atomic<float>, k_num_latents> scale;
    for (size_t i = 0; i < k_num_latents; ++i) {
        bias[i].store(0.0f);
        scale[i].store(1.0f);
    }

    anira::Context context{anira::ContextConfig{}};
    Handler encoder;
    Handler decoder;
    size_t encoder_latency_frames = 0;
    size_t latency = 0;
    try {
        anira::ContractHandle contract = anira::ContractHandle::from_file(step_dir + "/contract.json");

        // ---- TODO 1: the encoder -------------------------------------------
        create(encoder, context, step_dir + "/encoder.json");
        encoder_latency_frames = prepare(encoder, contract);  // In latent frames: its output's unit

        // ---- TODO 3: the decoder -------------------------------------------
        create(decoder, context, step_dir + "/decoder.json");
        const size_t decoder_latency = prepare(decoder, contract);  // In samples, on its own

        // The decoder's frames arrive late by the encoder's latency. Declared on
        // the decoder's output, the sum is what the host is told, and what the
        // decoder's ring is primed to: the frame has time to arrive.
        latency = encoder_latency_frames * k_frame_size + decoder_latency;
        contract.hard_latency("audio", static_cast<uint32_t>(latency));
        if (prepare(decoder, contract) != latency) { throw std::runtime_error("the declared latency was not taken"); }
        // --------------------------------------------------------------------
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                latency,
                1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                k_host.m_sample_rate);

    // Two tensors, built once: the host's block [1, samples] and one latent frame [8, 1]
    int64_t block_shape[2] = {1, 0};
    anira_tensor block{};
    std::array<float, k_num_latents> frame_data{};
    const int64_t frame_shape[2] = {k_num_latents, 1};
    anira_tensor frame{};
    anira_tensor_init_host(&frame, frame_data.data(), ANIRA_DTYPE_F32, 2, frame_shape);

    std::vector<size_t> late;
    size_t position = 0;
    size_t frames = 0;

    const std::vector<float> output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host.m_block_size,
        [&](float* samples, size_t num_samples) {
            block_shape[1] = static_cast<int64_t>(num_samples);
            anira_tensor_init_host(&block, samples, ANIRA_DTYPE_F32, 2, block_shape);

            // ---- TODO 1: the encoder ---------------------------------------
            // Audio in. A latent frame comes out once a whole block has been
            // encoded, about one latency later; usually none in this callback.
            anira_handler_push_data(encoder.m_handle, &block, 0);
            size_t delivered = 0;
            while (anira_handler_pop_data(encoder.m_handle, &frame, 0, &delivered) == ANIRA_OK && delivered == 1) {
                // The encoder's first frames are its latency priming: zeros. A
                // zero latent is not silence, it is a sound of its own, so
                // those frames must not reach the decoder.
                if (frames++ < encoder_latency_frames) { continue; }

                // ---- TODO 2: bias and scale on z ---------------------------
                for (size_t i = 0; i < k_num_latents; ++i) {
                    frame_data[i] = frame_data[i] * scale[i].load() + bias[i].load();
                }

                // ---- TODO 3: the decoder -----------------------------------
                anira_handler_push_data(decoder.m_handle, &frame, 0);
            }
            // The decoder's audio out, for this callback: samples, or the miss policy
            const anira_status status = anira_handler_pop_data(decoder.m_handle, &block, 0, &delivered);
            if (status == ANIRA_MISSED) { late.push_back(position); }
            // ----------------------------------------------------------------
            position += num_samples;
        },
        k_host.m_sample_rate);

    std::printf("  latent frames                %zu, %zu of them priming\n", frames, encoder_latency_frames);

    // The reference carries the model's own delay, which the decoder declared:
    // it lines up after the rest, as in steps 7 and 9.
    return report_with_latency(output.data(), k_target_output_signal.data(), output.size(),
                               latency - k_model_latency, late, k_host.m_block_size);
}
