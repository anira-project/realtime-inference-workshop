// Step 10 — Encoder, latents, decoder
//
// Goal:   the model cut in the middle: two handlers on one context, and the
//         eight latents per block in between, where a bias and a scale per
//         dimension are the instrument.
// Given:  encoder.json, decoder.json, contract.json, the fake host, anira.
// Task:   fill in the three TODO banners, each in two places: the planning
//         on the main thread, and the callback.
// Check:  with zero bias and unit scale the output equals the forward model's
//         reference, shifted by the latency the decoder reports.
//
// It builds and runs as it is, and says what is still missing.

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
        // create() it from encoder.json and prepare() it with the contract.
        // Its latency comes back in latent frames: keep it in
        // encoder_latency_frames, the callback needs it.
        // --------------------------------------------------------------------

        // ---- TODO 3: the decoder -------------------------------------------
        // create() it from decoder.json and prepare() it: that latency is in
        // samples, and it assumes a frame is there when its window starts.
        // Ours arrives late by the encoder's latency, so declare the sum on
        // the decoder's output (contract.hard_latency("audio", …)) and prepare
        // again. What it reports then is the latency the host is told.
        // --------------------------------------------------------------------
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    if (encoder.m_handle == nullptr) {
        std::fprintf(stderr, "TODO 1: create and prepare the encoder first.\n");
        return 2;
    }
    if (decoder.m_handle == nullptr || latency == 0) {
        std::fprintf(stderr, "TODO 3: create and prepare the decoder, and declare the summed latency.\n");
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
            // Push the block (input slot 0). Then pop latent frames (output
            // slot 0) into `frame` while one is delivered, counting them in
            // `frames`. The encoder's first encoder_latency_frames frames are
            // its priming, zeros: a zero latent is a sound, not silence, so
            // skip those.
            //   For each real frame:
            // ---- TODO 2: bias and scale on z -------------------------------
            //   frame_data[i] = frame_data[i] * scale[i] + bias[i], the atomics
            //   loaded on this thread.
            // ---- TODO 3: the decoder ---------------------------------------
            //   Push the frame into the decoder (input slot 0). After the loop,
            //   pop the decoder's audio for this callback into `block` (output
            //   slot 0); ANIRA_MISSED means the miss policy filled it, record
            //   `position` in `late` then.
            // ----------------------------------------------------------------
            position += num_samples;
        },
        k_host.m_sample_rate);

    if (frames == 0) {
        std::fprintf(stderr, "TODO 1: no latent frame came out of the encoder.\n");
        return 2;
    }
    std::printf("  latent frames                %zu, %zu of them priming\n", frames, encoder_latency_frames);

    // The reference carries the model's own delay, which the decoder declared:
    // it lines up after the rest, as in steps 7 and 9.
    return report_with_latency(output.data(), k_target_output_signal.data(), output.size(),
                               latency - k_model_latency, late, k_host.m_block_size);
}
