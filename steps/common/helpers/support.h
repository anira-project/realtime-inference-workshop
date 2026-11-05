// Comparing and reporting, shared by the steps.
//
// Goal: keep main.cpp about the inference and nothing else.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <vector>

// Everything below this counts as the same signal. The reference comes from a
// different runtime (ONNX Runtime in Python), so the numbers are close but not
// bit-identical.
constexpr float k_tolerance = 1e-4f;

inline float max_abs_diff(const float* a, const float* b, size_t num_samples) {
    float worst = 0.0f;
    for (size_t i = 0; i < num_samples; ++i) { worst = std::fmax(worst, std::fabs(a[i] - b[i])); }
    return worst;
}

// One line per run, for the steps that check several configurations.
// @label: what was run, e.g. the host block size
// @diff: the worst difference that run produced
// Returns true if it is within tolerance.
inline bool report_line(const char* label, float diff) {
    const bool ok = diff <= k_tolerance;
    std::printf("  %-24s max abs diff %-10.3g %s\n", label, diff, ok ? "OK" : "<-- FAILED");
    return ok;
}

// Compares the two signals and returns what main should exit with.
// @got: what the model produced
// @want: what it should have produced
// @num_samples: samples in both
// @tolerance: the largest difference that still counts as the same signal
inline int report(const float* got, const float* want, size_t num_samples, float tolerance = k_tolerance) {
    const float worst = max_abs_diff(got, want, num_samples);

    if (worst <= tolerance) {
        std::printf("OK: max abs diff %.3g, within %g of the reference.\n", worst, tolerance);
        return 0;
    }
    std::printf("FAILED: max abs diff %.3g, tolerance %g.\n", worst, tolerance);
    return 1;
}

// Compares a signal against a reference that is `latency` samples earlier, and
// returns what main should exit with: what the model produced, lined up with
// what it should have produced. The first `latency` samples are the priming.
// @got: what came out, `latency` samples late
// @want: what it should be, undelayed
// @num_samples: samples in both
// @latency: how far `want` has to be pushed back to line up
inline int report_with_latency(const float* got,
                               const float* want,
                               size_t num_samples,
                               size_t latency) {
    if (latency >= num_samples) {
        std::printf("FAILED: a latency of %zu samples leaves nothing of %zu to compare.\n",
                    latency,
                    num_samples);
        return 1;
    }
    return report(got + latency, want, num_samples - latency);
}

// The same, leaving out the callbacks where a block came back late and went
// out as the miss policy's fill (silence, or the input): those are glitches, and what matters is that everything
// around them still lines up. Fails when more than a quarter were late.
// @late: the first sample of every callback the miss policy filled
// @block_size: samples per callback
inline int report_with_latency(const float* got,
                               const float* want,
                               size_t num_samples,
                               size_t latency,
                               const std::vector<size_t>& late,
                               size_t block_size) {
    if (latency >= num_samples) {
        std::printf("FAILED: a latency of %zu samples leaves nothing of %zu to compare.\n",
                    latency,
                    num_samples);
        return 1;
    }
    float worst = 0.0f;
    size_t next = 0;
    for (size_t i = latency; i < num_samples; ++i) {
        while (next < late.size() && i >= late[next] + block_size) { ++next; }
        if (next < late.size() && i >= late[next]) { continue; }
        worst = std::fmax(worst, std::fabs(got[i] - want[i - latency]));
    }
    const size_t callbacks = num_samples / block_size;
    if (!late.empty()) {
        std::printf("  late callbacks, left out     %zu of %zu\n", late.size(), callbacks);
    }
    if (late.size() * 4 > callbacks) {
        std::printf("FAILED: more than a quarter of the callbacks were late.\n");
        return 1;
    }
    if (worst <= k_tolerance) {
        std::printf("OK: max abs diff %.3g, within %g of the reference.\n", worst, k_tolerance);
        return 0;
    }
    std::printf("FAILED: max abs diff %.3g, tolerance %g.\n", worst, k_tolerance);
    return 1;
}

// How far a host's output is from a reference that is `latency` samples
// earlier: the first `latency` samples have to be silence, the rest the
// reference. Returns the worst difference, or infinity if the start is not
// silent. Prints nothing.
inline float diff_with_latency(const float* got,
                               const float* want,
                               size_t num_samples,
                               size_t latency) {
    if (latency >= num_samples) { return std::numeric_limits<float>::infinity(); }
    for (size_t i = 0; i < latency; ++i) {
        if (got[i] != 0.0f) { return std::numeric_limits<float>::infinity(); }
    }
    return max_abs_diff(got + latency, want, num_samples - latency);
}

// Compares what a host heard with a reference that is `latency` samples
// earlier, the way a listener would: silence first, then the reference. Zeros
// are the processor's own silence and are not compared; a callback with zeros
// where the reference is not silent had a dropout, the worker was late. Every
// other sample has to be the reference. Fails on a wrong sample, or when more
// than a quarter of the callbacks had a dropout.
// @got: what the host heard
// @want: the reference, undelayed
// @num_samples: samples in both
// @latency: how late `got` is
// @block_size: samples per callback
inline int report_with_dropouts(const float* got,
                                const float* want,
                                size_t num_samples,
                                size_t latency,
                                size_t block_size) {
    if (latency >= num_samples) {
        std::printf("FAILED: a latency of %zu samples leaves nothing of %zu to compare.\n",
                    latency,
                    num_samples);
        return 1;
    }
    float worst = 0.0f;
    size_t wrong = 0;
    size_t dropouts = 0;
    const size_t callbacks = num_samples / block_size;
    for (size_t callback = 0; callback < callbacks; ++callback) {
        const size_t begin = callback * block_size;
        const size_t end = begin + block_size;
        auto expected = [&](size_t i) { return i < latency ? 0.0f : want[i - latency]; };

        // Zeros are silence the processor wrote, never a model sample: the
        // prefill, or a dropout. Where the reference is not silent, they are a
        // dropout; everything else has to be the reference.
        bool dropout = false;
        for (size_t i = begin; i < end; ++i) {
            if (got[i] == 0.0f) {
                if (std::fabs(expected(i)) > k_tolerance) { dropout = true; }
                continue;
            }
            const float diff = std::fabs(got[i] - expected(i));
            worst = std::fmax(worst, diff);
            if (diff > k_tolerance) { ++wrong; }
        }
        if (dropout) { ++dropouts; }
    }
    if (dropouts > 0) {
        std::printf("  callbacks with a dropout     %zu of %zu\n", dropouts, callbacks);
    }
    if (wrong > 0) {
        std::printf(
            "FAILED: %zu samples are neither the reference nor silence, max abs diff %.3g.\n",
            wrong,
            worst);
        return 1;
    }
    if (dropouts * 4 > callbacks) {
        std::printf("FAILED: more than a quarter of the callbacks had a dropout.\n");
        return 1;
    }
    std::printf("OK: max abs diff %.3g, within %g of the reference.\n", worst, k_tolerance);
    return 0;
}
