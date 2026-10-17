// Comparing and reporting, shared by the steps.
//
// Goal: keep main.cpp about the inference and nothing else.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

// Everything below this counts as the same signal. The reference comes from a
// different runtime (ONNX Runtime in Python), so the numbers are close but not
// bit-identical.
constexpr float k_tolerance = 1e-4f;

// The last line of an exception message: LibTorch wraps the actual error in a
// whole TorchScript traceback, and the reason is at the bottom of it.
inline std::string error_summary(const std::string& message) {
    const size_t end = message.find_last_not_of("\n ");
    if (end == std::string::npos) { return message; }
    const size_t start = message.rfind('\n', end);
    return start == std::string::npos ? message : message.substr(start + 1, end - start);
}

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
inline int report(const float* got, const float* want, size_t num_samples) {
    const float worst = max_abs_diff(got, want, num_samples);

    if (worst <= k_tolerance) {
        std::printf("OK: max abs diff %.3g, within %g of the reference.\n", worst, k_tolerance);
        return 0;
    }
    std::printf("FAILED: max abs diff %.3g, tolerance %g.\n", worst, k_tolerance);
    return 1;
}

// Compares a signal against a reference that is `latency` samples earlier, and
// returns what main should exit with: what the model produced, lined up with
// what it should have produced. The first `latency` samples are the priming.
// @got: what came out, `latency` samples late
// @want: what it should be, undelayed
// @num_samples: samples in both
// @latency: how far `want` has to be pushed back to line up
inline int report_with_latency(const float* got, const float* want, size_t num_samples, size_t latency) {
    if (latency >= num_samples) {
        std::printf("FAILED: a latency of %zu samples leaves nothing of %zu to compare.\n", latency, num_samples);
        return 1;
    }
    return report(got + latency, want, num_samples - latency);
}

// The same, leaving out the callbacks where a block came back late and went
// out as silence: those are glitches, and what matters is that everything
// around them still lines up. Fails when more than a quarter were late.
// @late: the first sample of every callback that went out as silence
// @block_size: samples per callback
inline int report_with_latency(const float* got, const float* want, size_t num_samples, size_t latency,
                               const std::vector<size_t>& late, size_t block_size) {
    if (latency >= num_samples) {
        std::printf("FAILED: a latency of %zu samples leaves nothing of %zu to compare.\n", latency, num_samples);
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
        std::printf("  late callbacks, silent       %zu of %zu\n", late.size(), callbacks);
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
