// Step 4 — Minimal C++ inference, reference implementation
//
// Goal:  load the model with LibTorch and run the test signal through it.
// Given: helpers/libtorch_engine.h, the engine.
// Check: the output within 1e-4 of what the model produced in Python.

#include <array>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

#include "helpers/libtorch_engine.h"
#include "helpers/support.h"
#include "helpers/target_signal.h"
#include "helpers/test_signal.h"

// What the export fixed, from its metadata. Sample rate and channel count are
// not used here — they are what the model assumes about the audio it is given.
// @m_path: absolute path to the exported model, set via CMake with
//          -DWORKSHOP_MODEL=/path/to/forward_stateful.pt; see models/README.md
// @m_input_size: samples the model takes per forward pass (the export calls it the quantum)
// @m_sample_rate: the sample rate the model was trained for
// @m_channels: channels in and out
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 48000.0;
    int m_channels = 1;
} k_model{};

int main() {
    // ---- TODO 1: create the engine -----------------------------------------
    // Once, outside the loop: loading is slow, and the model carries the state
    // from block to block. A fresh model per block would lose it.
    std::unique_ptr<workshop::LibTorchEngine> engine;
    try {
        engine = std::make_unique<workshop::LibTorchEngine>(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    constexpr size_t k_test_signal_length = workshop::k_signal_length;
    const std::array<float, k_test_signal_length>& input = workshop::k_input_signal;
    const std::array<float, k_test_signal_length>& target = workshop::k_target_output_signal;

    static_assert(input.size() == target.size(), "the target was generated for another signal");

    // ---- TODO 2: in what size to process ------------------------------------
    // Not a free choice: the export fixed it. The model was traced for
    // k_model.m_input_size samples per forward pass and refuses anything else.
    // Whole blocks only: the last k_test_signal_length % process_size samples
    // would stay unprocessed.
    const size_t process_size = static_cast<size_t>(k_model.m_input_size);
    const size_t num_blocks = k_test_signal_length / process_size;

    // The whole signal in one buffer, processed in place block by block.
    std::vector<float> output(input.begin(), input.end());

    try {
        for (size_t i = 0; i < num_blocks; ++i) {
            // ---- TODO 3: run the block through the engine ------------------
            engine->process(output.data() + i * process_size, process_size);
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr,
                     "process() failed: %s\n",
                     workshop::error_summary(error.what()).c_str());
        return 2;
    }

    return workshop::report(output.data(), target.data(), output.size());
}
