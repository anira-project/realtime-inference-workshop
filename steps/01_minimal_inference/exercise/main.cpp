// Step 1 — Minimal C++ inference
//
// Goal:   load the model with LibTorch and run the test signal through it.
// Given:  common/libtorch_engine.h, the engine — read it first.
// You do: wire it up, at the three TODO banners below.
// Check:  the output within 1e-4 of what the model produced in Python.
//
// It builds and runs as it is, and says what is still missing.

#include <array>
#include <cstddef>
#include <cstdio>
#include <vector>

#include "common/libtorch_engine.h"
#include "common/support.h"
#include "common/target_signal.h"
#include "common/test_signal.h"

// What the export fixed, from its metadata. Sample rate and channel count are
// not used here — they are what the model assumes about the audio it is given.
// @m_path: absolute path to the exported model, set via CMake with
//          -DWORKSHOP_MODEL=/path/to/forward_stateful.pt; see models/README.md
// @m_input_size: samples the model takes per forward pass
// @m_sample_rate: the sample rate the model was trained for
// @m_channels: channels in and out
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 48000.0;
    int m_channels = 1;
} k_model{};

int main() {
    std::unique_ptr<workshop::LibTorchEngine> engine;
    try {
        // ---- TODO 1 --------------------------------------------------------
        // Create the engine with k_model.m_path. It throws if the file is not
        // there, which is what the catch is for.
        // Note where this sits: outside the loop below, and why that matters.
        // --------------------------------------------------------------------
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;

    }

    if (engine == nullptr) {
        std::fprintf(stderr, "TODO 1: construct engine first with path to model");
        return 2;
    }

    constexpr size_t k_test_signal_length = workshop::k_signal_length;
    const std::array<float, k_test_signal_length>& input = workshop::k_input_signal;
    const std::array<float, k_test_signal_length>& target = workshop::k_target_output_signal;

    static_assert(input.size() == target.size(), "the target was generated for another signal");

    // ---- TODO 2 ------------------------------------------------------------
    // In what size do you want to process the signal? Pick a number, run it,
    // and see whether the model agrees with you. Whole blocks only: the last
    // k_test_signal_length % process_size samples stay unprocessed.
    // ------------------------------------------------------------------------
    const size_t process_size = 0;

    if (process_size == 0) {
        std::fprintf(stderr, "TODO 2: pick a size to process in first.\n");
        return 2;
    }
    const size_t num_blocks = k_test_signal_length / process_size;

    // The whole signal in one buffer, processed in place block by block.
    std::vector<float> output(input.begin(), input.end());

    try {
        for (size_t i = 0; i < num_blocks; ++i) {
            // ---- TODO 3 ----------------------------------------------------
            // Run this block through the engine: it starts at
            // output.data() + i * process_size and is process_size samples
            // long. It is processed in place, so afterwards `output` holds the
            // model's output for this block.
            // ----------------------------------------------------------------
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr,
                     "process() failed: %s\n",
                     workshop::error_summary(error.what()).c_str());
        return 2;
    }

    return workshop::report(output.data(), target.data(), output.size());
}