// Step 2 — Stateless and stateful
//
// Goal:   run the whole test signal through the model, block by block.
// Given:  common/helpers/onnx_engine.h, the engine from step 1.
// Task:   pass the engine everything the model needs, at the TODO banners below.
// Check:  every block within 1e-4 of what the model produced in Python.
//
// It builds once every ? is filled in. Without the state, every block after
// the first fails.

#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/step02.h"
#include "common/helpers/onnx_engine.h"

// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_state_size: floats in the model's state
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
} k_model{};

int main() {
    try {
        OnnxEngine engine(k_model.m_path);

        const size_t num_blocks = k_signal_length / k_model.m_input_size;

        // The whole signal, processed in place block by block.
        std::vector<float> audio(k_input_signal.begin(), k_input_signal.end());

        // ---- TODO 1 --------------------------------------------------------
        // What is the state size, and how should it be initialized?
        //
        // Hint: look at the struct above
        // --------------------------------------------------------------------
        std::vector<float> state(?, ?);  // <- fill out

        for (size_t i = 0; i < num_blocks; ++i) {
            // ---- TODO 2 ----------------------------------------------------
            // What should be passed next to audio in process()?
            // ----------------------------------------------------------------
            engine.process(audio.data() + i * k_model.m_input_size, k_model.m_input_size,
                           ?, ?);  // <- fill out
        }

        return report_blocks(audio, k_target_output_signal, k_model.m_input_size);
    } catch (const std::exception& error) {
        std::printf("FAILED: %s\n", error.what());
        return 1;
    }
}
