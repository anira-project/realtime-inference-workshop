// Step 1 — Minimal C++ inference
//
// Goal:   load the model with ONNX Runtime and run one block through it.
// Given:  common/helpers/onnx_engine.h, the engine — read it first.
// Task:   wire it up, at the three TODO banners below.
// Check:  the block within 1e-6 of what the model produced in Python.
//
// It builds and runs as it is, and says what is still missing.

#include <cstddef>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/step01.h"
#include "common/helpers/onnx_engine.h"

// Model settings taken from the export metadata.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
} k_model{};

int main() {
    try {
        // ---- TODO 1 --------------------------------------------------------
        // Set the path to the model, from k_model.m_path.
        //
        // Hint: look at the struct above
        // --------------------------------------------------------------------
        const std::string model_path = "";  // <- fill out

        // ---- TODO 2 --------------------------------------------------------
        // How many samples do you want to process? Pick a number, run it, and
        // see whether the model agrees with you.
        //
        // Hint: a model often has a fixed processing block size
        // --------------------------------------------------------------------
        const size_t process_size = 0;  // <- fill out

        check_todos(model_path, process_size);  // Given: names a missing TODO

        OnnxEngine engine(model_path);

        // The whole signal is in the buffer, but only its first process_size
        // samples get processed.
        std::vector<float> audio(k_input_signal.begin(), k_input_signal.end());

        // ---- TODO 3 --------------------------------------------------------
        // Run the first block through the engine: pass where it starts,
        // audio.data(), and process_size. It is processed in place, so
        // afterwards the first process_size samples of `audio` hold the
        // model's output.
        // --------------------------------------------------------------------
        // engine.process(?, ?);  // <- fill out

        return report_first_block(audio, k_input_signal, k_target_output_signal, process_size);
    } catch (const std::exception& error) {
        std::printf("FAILED: %s\n", error.what());
        return 1;
    }
}
