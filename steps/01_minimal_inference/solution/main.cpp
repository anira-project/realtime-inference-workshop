// Step 1 — Minimal C++ inference, reference implementation
//
// Goal:   load the model with ONNX Runtime and run one block through it.
// Given:  common/helpers/onnx_engine.h, the engine — read it first.
// Task:   wire it up, at the three TODO banners below.
// Check:  the block within 1e-6 of what the model produced in Python.
//
// It builds and runs as it is, and says what is still missing.

#include <array>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "common/helpers/onnx_engine.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

// Model settings taken from the export metadata. The sample rate and channel
// count are not used by this exercise, but describe the audio format expected
// by the model.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_sample_rate: the sample rate the model was trained for
// @m_channels: channels in and out
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 44100.0;
    int m_channels = 1;
} k_model{};

// One block is compared here, and it agrees far closer than the 1e-4 the longer
// signals are checked against.
constexpr float k_block_tolerance = 1e-6f;

int main() {

    // ---- TODO 1 --------------------------------------------------------
    // Set the path to the model, from k_model.m_path.
    //
    // Hint: look at the struct above
    // --------------------------------------------------------------------
    const std::string model_path = k_model.m_path; // <- solution

    // Only load what is there: an empty or a wrong path stops here.
    if (!std::filesystem::exists(model_path)) {
        std::fprintf(stderr, "TODO 1: set the path to the model first\n");
        return 2;
    }

    OnnxEngine engine(model_path);

    constexpr size_t k_test_signal_length = k_signal_length;
    const std::array<float, k_test_signal_length>& input = k_input_signal;
    const std::array<float, k_test_signal_length>& target = k_target_output_signal;

    static_assert(input.size() == target.size(), "the target was generated for another signal");

    // ---- TODO 2 ------------------------------------------------------------
    // How many samples do you want to process? Pick a number, run it, and see
    // whether the model agrees with you.
    //
    // Hint: a model often has a fixed processing block size
    // ------------------------------------------------------------------------
    const size_t process_size = k_model.m_input_size; // <- solution

    if (process_size == 0) {
        std::fprintf(stderr, "TODO 2: pick a size to process first.\n");
        return 2;
    }

    // The whole signal is in the buffer, but only its first process_size samples get processed.
    std::vector<float> audio(input.begin(), input.end());

    // ---- TODO 3 --------------------------------------------------------
    // Run the first block through the engine: pass `audio` and `process_size`.
    // It is processed in place, so afterwards the first process_size samples
    // of `audio` hold the model's output.
    // --------------------------------------------------------------------
    engine.process({audio}, {process_size}); // <- solution

    return report(audio.data(), target.data(), process_size, k_block_tolerance);
}
