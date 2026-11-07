// Step 4's reporting, given complete. Not part of the demonstration.
//
// Goal: turn the time of every single forward pass into the numbers the
//       slides read: the thresholds, the first forward passes of every
//       repetition, the statistics over all of them, the slowest forward passes,
//       and the worst case.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <vector>

// How many of every repetition's first forward passes are shown, and how many
// of the slowest are listed.
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
// @times: times[repetition][iteration], in milliseconds, in the order they ran
// @model_block_size: samples the model takes per forward pass
// @host_block_size: samples per host callback
// @sample_rate: in Hz
inline void report_forward_passes(const std::vector<std::vector<double>>& times,
                                  size_t model_block_size,
                                  size_t host_block_size,
                                  double sample_rate) {
    const size_t repetitions = times.size();
    const size_t iterations = times[0].size();

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

    // The same forward pass of every repetition side by side.
    std::printf("\nThe first %zu forward passes of %zu repetitions, in ms:\n          ",
                k_shown_first_passes,
                repetitions);
    for (size_t repetition = 0; repetition < repetitions; ++repetition) {
        std::printf("     rep %zu", repetition + 1);
    }
    for (size_t iteration = 0; iteration < k_shown_first_passes; ++iteration) {
        std::printf("\n  pass %2zu", iteration + 1);
        for (size_t repetition = 0; repetition < repetitions; ++repetition) {
            std::printf("  %8.2f", times[repetition][iteration]);
        }
    }
    std::printf("\n");

    // The mean is what a CPU meter shows; the rest is about the worst case.
    std::vector<double> all;
    for (const std::vector<double>& repetition : times) {
        all.insert(all.end(), repetition.begin(), repetition.end());
    }
    const double max_inference_ms = *std::max_element(all.begin(), all.end());
    std::printf("\nAll %zu forward passes:\n", all.size());
    std::printf("  mean    %7.2f ms\n",
                std::accumulate(all.begin(), all.end(), 0.0) / static_cast<double>(all.size()));
    std::printf("  median  %7.2f ms\n", percentile(all, 0.5));
    std::printf("  min     %7.2f ms\n", *std::min_element(all.begin(), all.end()));
    std::printf("  p99     %7.2f ms\n", percentile(all, 0.99));
    std::printf("  max     %7.2f ms\n", max_inference_ms);

    // Every forward pass, slowest first.
    std::vector<size_t> by_time(repetitions * iterations);
    std::iota(by_time.begin(), by_time.end(), size_t{0});
    std::sort(by_time.begin(), by_time.end(), [&](size_t a, size_t b) {
        return times[a / iterations][a % iterations] > times[b / iterations][b % iterations];
    });
    std::printf("\nThe %zu slowest of all %zu forward passes:\n", k_listed_passes, by_time.size());
    for (size_t i = 0; i < k_listed_passes; ++i) {
        const size_t repetition = by_time[i] / iterations;
        const size_t iteration = by_time[i] % iterations;
        std::printf("  repetition %zu, pass %3zu  %6.2f ms\n",
                    repetition + 1,
                    iteration + 1,
                    times[repetition][iteration]);
    }

    // The worst case: the number steps 6 and 7 build on. Longer than one model
    // block, and no latency is enough.
    const double model_block_ms = 1000.0 * static_cast<double>(model_block_size) / sample_rate;
    std::printf("\nYour worst case:\n");
    std::printf("  max inference time   %6.2f ms\n", max_inference_ms);
    if (max_inference_ms > model_block_ms) {
        std::printf(
            "\nLonger than one model block (%.1f ms): this machine cannot keep up with the\n"
            "model. Please tell us.\n",
            model_block_ms);
        return;
    }
    std::printf("\nWrite %.2f into k_max_inference_ms in steps/common/inference_time.h.\n",
                max_inference_ms);
}
