// Step 4 — Benchmarking the forward pass
//
// Goal:   find out how long one forward pass takes on your machine, and how long
//         it takes in the best and worst case.
// Given:  std::chrono, and the ONNX Runtime engine from step 2.
// Task:   none, this step is a demonstration. Build it, run it, read the numbers.
// Check:  the numbers, against the real-time threshold of a 512-sample callback,
//         and your worst case in samples: the number steps 6 and 7 need.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/checks/step04.h"
#include "common/helpers/onnx_engine.h"

// Model settings taken from the export metadata.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_state_size: floats in the model's state
// @m_sample_rate: the sample rate the model was trained for
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
    double m_sample_rate = 44100.0;
} k_model{};

// What the host hands over per callback, as in every step of the first part.
constexpr size_t k_host_block_size = 512;

// Measured as anira's benchmarks do: every repetition constructs a fresh engine,
// and every iteration times one single forward pass.
constexpr size_t k_repetitions = 2;
constexpr size_t k_iterations = 10;

int main() {
    std::vector<std::vector<double>> times_ms(k_repetitions, std::vector<double>(k_iterations));
    std::vector<float> model_block(k_model.m_input_size);
    std::vector<float> state(k_model.m_state_size);

    for (size_t repetition = 0; repetition < k_repetitions; ++repetition) {
        // A fresh engine and a zero state. Loading is slow and is not what we measure.
        OnnxEngine engine(k_model.m_path);
        std::fill(state.begin(), state.end(), 0.0f);

        for (size_t iteration = 0; iteration < k_iterations; ++iteration) {
            // The next block of the test signal, round and round.
            const size_t start = (iteration * k_model.m_input_size) % k_signal_length;
            std::copy_n(k_input_signal.data() + start, k_model.m_input_size, model_block.data());

            // Only this is timed.
            const auto begin = std::chrono::steady_clock::now();
            engine.process(model_block.data(), model_block.size(), state);
            const auto end = std::chrono::steady_clock::now();

            times_ms[repetition][iteration] =
                std::chrono::duration<double, std::milli>(end - begin).count();
        }
    }

    report_forward_passes(times_ms, k_model.m_input_size, k_host_block_size, k_model.m_sample_rate);
    return 0;
}
