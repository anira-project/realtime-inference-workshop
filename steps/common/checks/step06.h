// Step 6's checks, given complete. Not part of the exercise.
//
// Goal: say which TODO is still missing, and compare what the host heard
//       with the reference.
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/helpers/support.h"


// TODO 1, in prepare(): step 3's prefill, plus the worker's time to run the
// model block.
// @prefill: the samples of silence m_output starts with
// @max_block_size: the largest block the callback will be given
// @model_block_size: samples the model takes per forward pass
// @worker_samples: the worker's time, k_max_inference_samples_rounded
inline void check_prefill(size_t prefill,
                          size_t max_block_size,
                          size_t model_block_size,
                          size_t worker_samples) {
    const size_t needed = (model_block_size - max_block_size) + worker_samples;
    if (prefill < needed) {
        throw std::runtime_error(
            "TODO 1: the prefill is too small, the worker's first block would come too late");
    }
    if (prefill > needed) {
        throw std::runtime_error(
            "TODO 1: the prefill is too large, the host would hear the model later than needed");
    }
}

// TODO 2 and 3, then the comparison. Returns what main should exit with.
// @host_output: what the host heard
// @target: what the model produced in Python
// @prefill: how late the host hears the model's stream
// @host_block_size: samples per callback
// @sample_rate: in Hz
inline int report_with_prefill(const std::vector<float>& host_output,
                               const std::array<float, k_signal_length>& target,
                               size_t prefill,
                               size_t host_block_size,
                               double sample_rate) {
    if (std::all_of(host_output.begin(), host_output.end(), [](float x) { return x == 0.0f; })) {
        throw std::runtime_error(
            "TODO 2 or 3: the host only got silence. Does the audio thread push into m_input,\n"
            "and does the worker pop a model block, run the model and push into m_output?");
    }

    std::printf("  %-28s %zu samples (%.1f ms)\n",
                "prefill",
                prefill,
                1000.0 * static_cast<double>(prefill) / sample_rate);

    // What the host heard: the prefill in silence, then the model's stream,
    // apart from callbacks where the worker was late and zeros went out.
    const int result = report_with_dropouts(host_output.data(),
                                            target.data(),
                                            host_output.size(),
                                            prefill,
                                            host_block_size);
    if (result != 0) {
        std::printf(
            "Dropouts or wrong samples mean the worker was late: raise k_max_inference_ms in\n"
            "common/inference_time.h to the worst case of a few step 4 runs.\n");
        return result;
    }
    std::printf(
        "OK: inference ran on the worker thread, the host got the model's stream back\n"
        "%zu samples late.\n",
        prefill);
    return 0;
}
