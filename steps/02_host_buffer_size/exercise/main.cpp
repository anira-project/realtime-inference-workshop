// Step 2 — Model input size vs. host buffer size
//
// Goal:   feed a model that only takes 2048 samples from a host that hands you
//         whatever it likes.
// Given:  common/ring_buffer.h and common/host.h — read both first.
// You do: fill in run_at_block_size(), at the three TODO banners.
// Check:  the stream the model produced is the same for every host block size.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/host.h"
#include "common/libtorch_engine.h"
#include "common/ring_buffer.h"
#include "common/support.h"
#include "common/target_signal.h"
#include "common/test_signal.h"

// What the export fixed, from its metadata. Sample rate and channel count are
// not used here — they are what the model assumes about the audio it is given.
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 48000.0;
    int m_channels = 1;
} k_model{};

// Block sizes a host might pick. 2048 is the one case where it happens to match
// the model; the rest are ordinary, and one of them does not even divide it.
constexpr std::array<size_t, 6> k_host_block_sizes = {64, 128, 480, 512, 1024, 2048};

namespace {

// Runs the whole signal through the host at one block size, and returns
// everything the model produced, in order.
// @engine: the model, which only accepts model_input_size samples
// @signal: the input
// @host_block_size: what the host hands over per callback
// @model_input_size: what the model takes per forward pass
std::vector<float> run_at_block_size(workshop::LibTorchEngine& engine,
                                     const std::vector<float>& signal,
                                     size_t host_block_size,
                                     size_t model_input_size) {
    // ---- TODO 1 ------------------------------------------------------------
    // How much room does each ring buffer need? Think about the worst moment:
    // the host has just written a block and the model has not taken anything
    // out yet. Too little and push() throws, which the report will show you.
    // --------------------------------------------------------------------------
    workshop::RingBuffer input(0);
    workshop::RingBuffer output(0);

    std::vector<float> block(model_input_size);  // One model block, reused
    std::vector<float> produced;

    workshop::run_host(signal.data(),
                       signal.size(),
                       host_block_size,
                       [&](float* samples, size_t num_samples) {
                           // ---- TODO 2 ----------------------------------------------------
                           // Take the host's samples in, and run the model whenever a whole
                           // block has arrived. `block` is there to hold one model block.
                           // Append every block the model returns to `produced` — that is
                           // what the check reads.
                           // ------------------------------------------------------------------

                           // ---- TODO 3 ----------------------------------------------------
                           // Give the host its num_samples back, in place. At the start the
                           // model has not produced anything yet — so what goes out?
                           // ------------------------------------------------------------------
                       });

    return produced;
}

}  // namespace

int main() {
    std::unique_ptr<workshop::LibTorchEngine> engine;
    try {
        engine = std::make_unique<workshop::LibTorchEngine>(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    const std::vector<float> input(workshop::k_input_signal.begin(),
                                   workshop::k_input_signal.end());
    const std::array<float, workshop::k_signal_length>& target = workshop::k_target_output_signal;
    const auto model_input_size = static_cast<size_t>(k_model.m_input_size);

    bool all_ok = true;
    for (const size_t host_block_size : k_host_block_sizes) {
        engine->reset();  // Every run starts from the same state

        const std::string label = "host block " + std::to_string(host_block_size);
        std::vector<float> produced;
        try {
            produced = run_at_block_size(*engine, input, host_block_size, model_input_size);
        } catch (const std::exception& error) {
            std::printf("  %-24s %s\n",
                        label.c_str(),
                        workshop::error_summary(error.what()).c_str());
            all_ok = false;
            continue;
        }

        // The model saw whole blocks, so what it produced has to match the
        // reference for as far as it got — and it has to have got that far.
        const size_t host_samples = input.size() / host_block_size * host_block_size;
        const size_t expected = host_samples / model_input_size * model_input_size;
        if (produced.size() != expected) {
            std::printf("  %-24s produced %zu samples, expected %zu   <-- FAILED\n",
                        label.c_str(),
                        produced.size(),
                        expected);
            all_ok = false;
            continue;
        }

        all_ok &= workshop::report_line(
            label.c_str(),
            workshop::max_abs_diff(produced.data(), target.data(), produced.size()));
    }

    if (all_ok) {
        std::printf("\nOK: the model saw the same stream at every host block size.\n");
        return 0;
    }
    std::printf("\nFAILED.\n");
    return 1;
}
