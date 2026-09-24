// Comparing and reporting, shared by the steps.
//
// Goal: keep main.cpp about the inference and nothing else.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

namespace workshop {

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

}  // namespace workshop
