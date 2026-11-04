// Step 2's checks, given complete. Not part of the exercise.
//
// Goal: compare the signal with what the model produced in Python, block by
//       block, to see where it goes wrong.
#pragma once

#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/helpers/support.h"

// One line per block, then the whole signal. Returns what main should exit with.
// @audio: the whole signal, processed in place
// @target: what the model produced in Python
// @process_size: samples per block
inline int report_blocks(const std::vector<float>& audio,
                         const std::array<float, k_signal_length>& target,
                         size_t process_size) {
    bool later_blocks_fail = false;
    bool first_block_ok = true;
    for (size_t start = 0; start < audio.size(); start += process_size) {
        const std::string label = "block " + std::to_string(start / process_size);
        const bool ok = report_line(
            label.c_str(), max_abs_diff(audio.data() + start, target.data() + start, process_size));
        if (start == 0) {
            first_block_ok = ok;
        } else if (!ok) {
            later_blocks_fail = true;
        }
    }

    // Right at the start, wrong after it: every block starts from a zero state.
    if (first_block_ok && later_blocks_fail) {
        std::printf("FAILED: TODO 1 and 2: block 0 is right, the blocks after it are not\n");
        return 1;
    }
    return report(audio.data(), target.data(), audio.size());
}
