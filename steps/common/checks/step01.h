// Step 1's checks, given complete. Not part of the exercise.
//
// Goal: say which TODO is still missing, then compare the first block with
//       what the model produced in Python.
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/helpers/support.h"

// One block is compared here, and it agrees far closer than the 1e-4 the longer
// signals are checked against.
constexpr float k_block_tolerance = 1e-6f;

// TODO 1 and 2, before the engine loads anything.
// @model_path: the path from TODO 1
// @process_size: the size from TODO 2
inline void check_todos(const std::string& model_path, size_t process_size) {
    if (!std::filesystem::exists(model_path)) {
        throw std::runtime_error("TODO 1: set the path to the model first");
    }
    if (process_size == 0) {
        throw std::runtime_error("TODO 2: pick a size to process first");
    }
}

// TODO 3, then the comparison. Returns what main should exit with.
// @audio: the signal, its first block processed in place
// @input: what went in
// @target: what the model produced in Python
// @process_size: samples in that block
inline int report_first_block(const std::vector<float>& audio,
                              const std::array<float, k_signal_length>& input,
                              const std::array<float, k_signal_length>& target,
                              size_t process_size) {
    if (std::equal(audio.data(), audio.data() + process_size, input.data())) {
        throw std::runtime_error("TODO 3: run the first block through the engine");
    }
    return report(audio.data(), target.data(), process_size, k_block_tolerance);
}
