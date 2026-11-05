// Step 7's checks, given complete. Not part of the exercise.
//
// Goal: say which TODO is still missing, and check that dry and wet line up
//       in the mix, both late by exactly the reported latency.
#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/helpers/ring_buffer.h"
#include "common/helpers/support.h"

// Both TODOs, in prepare().
// @latency: what the host is told
// @model_latency: the model's own delay
// @dry: the dry ring buffer, after TODO 2
inline void check_todos(size_t latency, size_t model_latency, const RingBuffer& dry) {
    if (latency == 0) { throw std::runtime_error("TODO 1: work out the latency"); }
    if (latency < model_latency) {
        throw std::runtime_error("TODO 1: the latency is shorter than the model's own");
    }
    if (dry.available() != latency) {
        throw std::runtime_error(
            "TODO 2: delay the dry signal: m_dry has to start with the latency in silence");
    }
}

// The mix the host heard, against dry and wet references, both late by the
// reported latency. Returns what main should exit with.
// @got: what the host heard
// @input: what the host sent: the dry reference
// @target: what the model produced in Python: the wet reference, with the
//          model's own delay already in it
// @mix: 0 is dry only, 1 is wet only
// @latency: what the host is told
// @model_latency: the model's own delay
// @host_block_size: samples per callback
// @sample_rate: in Hz
inline int report_mix(const std::vector<float>& got,
                      const std::array<float, k_signal_length>& input,
                      const std::array<float, k_signal_length>& target,
                      float mix,
                      size_t latency,
                      size_t model_latency,
                      size_t host_block_size,
                      double sample_rate) {
    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n",
                latency,
                1000.0 * static_cast<double>(latency) / sample_rate,
                sample_rate);

    // Where each path should be: the dry one by the whole latency, the wet one
    // by the rest, since the model's own delay is already in the reference.
    const size_t wet_delay = latency - model_latency;
    auto dry_at = [&](size_t i) { return i < latency ? 0.0f : input[i - latency]; };
    auto wet_at = [&](size_t i) { return i < wet_delay ? 0.0f : target[i - wet_delay]; };

    // Where the worker was late, a sample has only its dry part: a dropout.
    // Every other sample has to be the mix of both.
    float worst = 0.0f;
    size_t wrong = 0;
    size_t dropouts = 0;
    const size_t callbacks = got.size() / host_block_size;
    for (size_t callback = 0; callback < callbacks; ++callback) {
        bool dropout = false;
        for (size_t i = callback * host_block_size; i < (callback + 1) * host_block_size; ++i) {
            const float dry = (1.0f - mix) * dry_at(i);
            const float mixed_diff = std::fabs(got[i] - (dry + mix * wet_at(i)));
            if (mixed_diff <= k_tolerance) {
                worst = std::fmax(worst, mixed_diff);
            } else if (std::fabs(got[i] - dry) <= k_tolerance) {
                dropout = true;
            } else {
                ++wrong;
            }
        }
        if (dropout) { ++dropouts; }
    }

    if (dropouts > 0) {
        std::printf("  callbacks with a dropout     %zu of %zu\n", dropouts, callbacks);
    }
    if (wrong > 0) {
        std::printf(
            "FAILED: dry and wet do not line up in %zu samples: the reported latency does not\n"
            "match what comes out.\n",
            wrong);
        return 1;
    }
    if (4 * dropouts > callbacks) {
        std::printf(
            "FAILED: the worker was late in more than a quarter of the callbacks: raise\n"
            "k_max_inference_ms in common/inference_time.h to the worst case of a few step 4 "
            "runs.\n");
        return 1;
    }
    std::printf(
        "OK: max abs diff %.3g at mix %.1f: dry and wet are both late by exactly the\n"
        "reported latency, so they line up.\n",
        worst,
        mix);
    return 0;
}
