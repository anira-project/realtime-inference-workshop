// Step 2 — Stateless and stateful
//
// Goal:   run the whole test signal through the model, block by block.
// Given:  common/helpers/onnx_engine.h, the engine from step 1.
// Task:   pass the engine everything the model needs, at the TODO banners below.
// Check:  every block within 1e-4 of what the model produced in Python.
//
// It builds and runs as it is, and fails from the second block on.

#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "common/helpers/onnx_engine.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_state_size: floats in the model's state
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    int m_input_size = 2048;
    size_t m_state_size = 63488;
} k_model{};

int main() {
    OnnxEngine engine(k_model.m_path);

    constexpr size_t k_test_signal_length = k_signal_length;
    const std::array<float, k_test_signal_length>& input = k_input_signal;
    const std::array<float, k_test_signal_length>& target = k_target_output_signal;

    const size_t process_size = k_model.m_input_size;
    const size_t num_blocks = k_test_signal_length / process_size;

    // The whole signal, processed in place block by block.
    std::vector<float> audio(input.begin(), input.end());

    // ---- TODO 1 ------------------------------------------------------------
    // What is the state size, and how should it be initialized?
    // ------------------------------------------------------------------------
    // std::vector<float> state(?, ?); // <- fill out

    for (size_t i = 0; i < num_blocks; ++i) {
        // ---- TODO 2 --------------------------------------------------------
        // What should be passed to process()?
        // --------------------------------------------------------------------
        engine.process({audio}, {process_size}, i * process_size); // <- fill out
    }

    // Block by block, to see where it goes wrong.
    for (size_t i = 0; i < num_blocks; ++i) {
        const size_t start = i * process_size;
        const std::string label = "block " + std::to_string(i);
        report_line(label.c_str(), max_abs_diff(audio.data() + start, target.data() + start, process_size));
    }

    return report(audio.data(), target.data(), audio.size());
}
