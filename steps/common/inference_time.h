// Your machine's inference time, from step 4. Steps 6, 7 and the plugin read
// it from here.
#pragma once

#include <cmath>
#include <cstddef>

// The largest worst case that can work at all: one model block of time, 2048
// samples at 44.1 kHz. A forward pass that takes longer can never keep up,
// whatever the latency.
constexpr double k_worst_case_inference_ms = 46.4;

// Your worst case, from the last line of step 4, in ms. It
// starts at the worst case above, so everything runs; your number makes the
// latency smaller.
constexpr double k_max_inference_ms = k_worst_case_inference_ms;  // <- fill out after step 4

constexpr double k_max_inference_samples = k_max_inference_ms * 44.1;

// Rounded up to whole 512-sample callbacks, because the audio thread only picks
// results up once per callback.
inline const size_t k_max_inference_samples_rounded =
    static_cast<size_t>(std::ceil(k_max_inference_samples / 512.0)) * 512;

static_assert(k_max_inference_ms > 0.0 && k_max_inference_ms <= k_worst_case_inference_ms,
              "k_max_inference_ms is larger than one model block: this machine cannot keep up "
              "with the model. Please tell us.");
