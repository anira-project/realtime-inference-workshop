// Step 1's checks, given complete. Not part of the exercise.
//
// Goal: say whether TODO 2 is still missing, then compare the first block
//       with what the model produced in Python.
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/helpers/support.h"

// One block is compared here, and it agrees far closer than the 1e-4 the longer
// signals are checked against.
constexpr float k_block_tolerance = 1e-6f;

// TODO 2, then the comparison. Returns what main should exit with.
// @buffer: the signal, its first block processed in place
// @input: what went in
// @target: what the model produced in Python
// @process_size: samples in that block
inline int report_first_block(const std::vector<float>& buffer,
                              const std::array<float, k_signal_length>& input,
                              const std::array<float, k_signal_length>& target,
                              size_t process_size) {
    if (std::equal(buffer.data(), buffer.data() + process_size, input.data())) {
        throw std::runtime_error("TODO 2: run the first block through the engine");
    }
    return report(buffer.data(), target.data(), process_size, k_block_tolerance);
}
