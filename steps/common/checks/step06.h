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

// TODO 1, in prepare(), before anything is allocated.
// @max_inference_ms: k_max_inference_ms, your number from step 4
// @rounded_max_inference_time: the worker's time, in samples
// @prefill: the samples of silence m_output starts with
// @output_capacity: the size of m_output
// @max_block_size: the largest block the callback will be given
// @sample_rate: in Hz
inline void check_todos(double max_inference_ms,
                         size_t rounded_max_inference_time,
                         size_t prefill,
                         size_t output_capacity,
                         size_t max_block_size,
                         double sample_rate) {
    if (max_inference_ms <= 0.0) {
        throw std::runtime_error("put your worst case from step 4 into k_max_inference_ms");
    }
    const double max_inference_samples = max_inference_ms * sample_rate / 1000.0;
    if (rounded_max_inference_time == 0 || rounded_max_inference_time % max_block_size != 0 ||
        static_cast<double>(rounded_max_inference_time) < max_inference_samples) {
        throw std::runtime_error(
            "TODO 1: round k_max_inference_ms up to whole callbacks, in samples");
    }
    if (prefill == 0) { throw std::runtime_error("TODO 1: work out the prefill"); }
    if (output_capacity < prefill + max_block_size) {
        throw std::runtime_error("TODO 1: give m_output room for the prefill and a callback more");
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
            "Dropouts or wrong samples mean the worker was late: raise k_max_inference_ms to\n"
            "the worst case of a few step 4 runs. If it is already that, the machine is busy.\n");
        return result;
    }
    std::printf(
        "OK: inference ran on the worker thread, the host got the model's stream back\n"
        "%zu samples late.\n",
        prefill);
    return 0;
}
