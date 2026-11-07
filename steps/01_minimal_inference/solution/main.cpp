// Step 1 — Minimal C++ inference, reference implementation
//
// Goal:   load the model with ONNX Runtime and run one block through it.
// Given:  common/helpers/onnx_engine.h, the engine — read it first.
// Task:   wire it up, at the two TODO banners below.
// Check:  the block within 1e-6 of what the model produced in Python.
//
// It builds once every ? is filled in, then says what is still off.

#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/step01.h"
#include "common/helpers/onnx_engine.h"

// Model settings taken from the export metadata.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
struct ModelConfig {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
};
constexpr ModelConfig k_model{};

int main() {
    try {
        // ---- TODO 1 --------------------------------------------------------
        // Load the model: load() takes the path to the .onnx file.
        //
        // Hint: look at the struct above
        // --------------------------------------------------------------------
        OnnxEngine engine;
        engine.load(k_model.m_path);  // <- solution

        // The whole signal is in the buffer, but only its first block gets
        // processed.
        std::vector<float> buffer(k_input_signal.begin(), k_input_signal.end());

        // ---- TODO 2 --------------------------------------------------------
        // Run the first block through the engine: pass where it starts and how
        // many samples it has. Pick a number, run it, and see whether the
        // model agrees with you. It is processed in place, so afterwards the
        // first samples of `buffer` hold the model's output.
        //
        // Hint: a model often has a fixed processing block size
        // --------------------------------------------------------------------
        engine.process(buffer.data(), k_model.m_input_size);  // <- solution

        return report_first_block(buffer, k_input_signal, k_target_output_signal, k_model.m_input_size);
    } catch (const std::exception& error) {
        std::printf("FAILED: %s\n", error.what());
        return 1;
    }
}
