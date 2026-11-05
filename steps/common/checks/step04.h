// Step 4's reporting, given complete. Not part of the demonstration.
//
// Goal: turn the time of every single forward pass into the numbers the
//       slides read: the thresholds, the first forward passes of every engine,
//       the statistics after the warm-up, the slowest forward passes, and the
//       worst case in samples.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <vector>

// The first two forward passes of every engine are a warm-up: they are slower
// than the rest, on LibTorch by far, because TorchScript optimises the model
// while it runs it. A plugin makes them before the audio starts, so they do not
// count towards the worst case.
constexpr size_t k_warm_up_passes = 2;

// How many of every engine's first forward passes are shown, and how many of
// the slowest are listed.
constexpr size_t k_shown_first_passes = 5;
constexpr size_t k_listed_passes = 5;

// The n-th percentile.
// @times: the measured times, in any order
// @fraction: 0.99 returns the time that 99 percent of the forward passes stayed under
inline double percentile(std::vector<double> times, double fraction) {
    std::sort(times.begin(), times.end());
    const auto index =
        static_cast<size_t>(std::lround(fraction * static_cast<double>(times.size() - 1)));
    return times[index];
}

// Prints everything step 4 reads off the measurement.
// @times: times[engine][pass], in milliseconds, in the order they ran
// @model_block_size: samples the model takes per forward pass
// @host_block_size: samples per host callback
// @sample_rate: in Hz
inline void report_forward_passes(const std::vector<std::vector<double>>& times,
                                  size_t model_block_size,
                                  size_t host_block_size,
                                  double sample_rate) {
    const size_t engines = times.size();
    const size_t passes = times[0].size();

    // How much time a block of audio is worth. The model needs a whole model
    // block before it can run at all, but the deadline belongs to the callback
    // it runs in, and the host's callback is shorter.
    std::printf("Real-time threshold at %.0f Hz:\n", sample_rate);
    std::printf("  model block  %5zu samples   %6.2f ms\n",
                model_block_size,
                1000.0 * static_cast<double>(model_block_size) / sample_rate);
    std::printf("  callback     %5zu samples   %6.2f ms\n",
                host_block_size,
                1000.0 * static_cast<double>(host_block_size) / sample_rate);

    // The same forward pass of every engine side by side: the warm-up comes
    // back with every engine.
    std::printf("\nThe first %zu forward passes of %zu engines, in ms:\n          ",
                k_shown_first_passes,
                engines);
    for (size_t engine = 0; engine < engines; ++engine) { std::printf("  engine %zu", engine + 1); }
    for (size_t pass = 0; pass < k_shown_first_passes; ++pass) {
        std::printf("\n  pass %2zu", pass + 1);
        for (size_t engine = 0; engine < engines; ++engine) {
            std::printf("  %8.2f", times[engine][pass]);
        }
    }
    std::printf("\n");

    // The mean is what a CPU meter shows; the rest is about the worst case.
    std::vector<double> warm;
    for (const std::vector<double>& engine : times) {
        warm.insert(warm.end(),
                    engine.begin() + static_cast<std::ptrdiff_t>(k_warm_up_passes),
                    engine.end());
    }
    const double max_inference_ms = *std::max_element(warm.begin(), warm.end());
    std::printf("\nAfter the warm-up, forward passes %zu to %zu of every engine (%zu passes):\n",
                k_warm_up_passes + 1,
                passes,
                warm.size());
    std::printf("  mean    %7.2f ms\n",
                std::accumulate(warm.begin(), warm.end(), 0.0) / static_cast<double>(warm.size()));
    std::printf("  median  %7.2f ms\n", percentile(warm, 0.5));
    std::printf("  min     %7.2f ms\n", *std::min_element(warm.begin(), warm.end()));
    std::printf("  p99     %7.2f ms\n", percentile(warm, 0.99));
    std::printf("  max     %7.2f ms\n", max_inference_ms);

    // Every forward pass, slowest first.
    std::vector<size_t> by_time(engines * passes);
    std::iota(by_time.begin(), by_time.end(), size_t{0});
    std::sort(by_time.begin(), by_time.end(), [&](size_t a, size_t b) {
        return times[a / passes][a % passes] > times[b / passes][b % passes];
    });
    std::printf("\nThe %zu slowest of all %zu forward passes:\n", k_listed_passes, by_time.size());
    for (size_t i = 0; i < k_listed_passes; ++i) {
        const size_t engine = by_time[i] / passes;
        const size_t pass = by_time[i] % passes;
        std::printf("  engine %zu, pass %3zu  %6.2f ms\n",
                    engine + 1,
                    pass + 1,
                    times[engine][pass]);
    }

    // The worst case after the warm-up, in samples: the time the worker in
    // step 6 has to be given, rounded up to whole host callbacks, because the
    // audio thread only picks results up once per callback.
    const double max_inference_samples = max_inference_ms * sample_rate / 1000.0;
    const auto rounded_max_inference_time =
        static_cast<size_t>(
            std::ceil(max_inference_samples / static_cast<double>(host_block_size))) *
        host_block_size;

    std::printf("\nYour worst case after the warm-up:\n");
    std::printf("  max inference time           %6.2f ms = %.0f samples\n",
                max_inference_ms,
                max_inference_samples);
    std::printf("  rounded_max_inference_time   %zu samples (whole %zu-sample callbacks)\n",
                rounded_max_inference_time,
                host_block_size);
    std::printf("\nPut %.2f into k_max_inference_ms in steps 6 and 7.\n", max_inference_ms);
}
