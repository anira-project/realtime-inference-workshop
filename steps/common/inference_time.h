// Your worst case from step 4, in ms: the number in its last line. Steps 6, 7
// and the plugin read it. Until you fill it in, it is 46.4 ms, one model block
// of time: everything runs, with more latency. Yours makes the latency smaller.
#pragma once

constexpr double k_max_inference_ms = 46.4;  // <- fill out after step 4

// More than one model block of time (2048 samples at 44.1 kHz) can never keep
// up, whatever the latency: the build stops.
static_assert(k_max_inference_ms > 0.0 && k_max_inference_ms <= 46.4,
              "k_max_inference_ms is larger than one model block: this machine cannot keep up "
              "with the model. Please tell us.");
