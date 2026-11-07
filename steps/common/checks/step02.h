// Step 2's checks, given complete. Not part of the exercise.
//
// Goal: compare the signal with what the model produced in Python, block by
//       block, to see where it goes wrong.
#pragma once

#include <array>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/helpers/support.h"

// TODO 1, before the first block: the engine gets a pointer and a size, and
// cannot see how large the vector really is. Too small, and it would read past
// its end.
// @state: the vector from TODO 1
// @state_size: the floats in the model's state
inline void check_state(const std::vector<float>& state, size_t state_size) {
    if (state.size() != state_size) {
        throw std::runtime_error("TODO 1: the state has " + std::to_string(state.size()) +
                                 " floats, the model's state is " + std::to_string(state_size));
    }
    for (const float value : state) {
        if (value != 0.0f) {
            throw std::runtime_error("TODO 1: the state starts at zero, before the first block");
        }
    }
}

// One line per block, then the whole signal. Returns what main should exit with.
// @buffer: the whole signal, processed in place
// @target: what the model produced in Python
// @process_size: samples per block
inline int report_blocks(const std::vector<float>& buffer,
                         const std::array<float, k_signal_length>& target,
                         size_t process_size) {
    bool later_blocks_fail = false;
    bool first_block_ok = true;
    for (size_t start = 0; start < buffer.size(); start += process_size) {
        const std::string label = "block " + std::to_string(start / process_size);
        const bool ok = report_line(
            label.c_str(), max_abs_diff(buffer.data() + start, target.data() + start, process_size));
        if (start == 0) {
            first_block_ok = ok;
        } else if (!ok) {
            later_blocks_fail = true;
        }
    }

    // Right at the start, wrong after it: every block starts from a zero state.
    if (first_block_ok && later_blocks_fail) {
        fail("TODO 2: block 0 is right, the blocks after it are not: pass the state with\n"
             "every block, so each one starts where the one before stopped");
        return 1;
    }
    return report(buffer.data(), target.data(), buffer.size());
}
