// Step 3 — Benchmarking the forward pass
//
// Goal:   find out how long one forward pass takes, and how long it takes in
//         the worst case.
// Given:  Google Benchmark, and the engine from step 1.
// Task:   fill in the three TODO banners.
// Check:  the numbers, against the budget a host block size gives you.
//
// It builds and runs as it is, and says which TODO is still open.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "common/libtorch_engine.h"
#include "common/test_signal.h"

// Model settings taken from the export metadata.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_sample_rate: the sample rate the model was trained for
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 48000.0;
} k_model{};

// How many times the forward pass is measured. Each repetition times one single
// call, so a percentile over these is a percentile over real calls.
constexpr int k_repetitions = 200;

namespace {

// The model is loaded once. Loading is slow and is not what we measure.
LibTorchEngine& engine() {
    static LibTorchEngine instance(k_model.m_path);
    return instance;
}

// What Google Benchmark measures. Everything inside the loop below is measured.
void forward_pass(benchmark::State& state) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> block(k_input_signal.begin(), k_input_signal.begin() + block_size);

    // A copy to compare against, so the exercise can tell whether the model ran.
    const std::vector<float> input_block(block);

    for (auto _ : state) {
        // ---- TODO 1 --------------------------------------------------------
        // Run one forward pass on `block`, with `block_size` samples. Only what
        // stands in this loop is timed, so nothing else belongs here.
        // --------------------------------------------------------------------

        benchmark::DoNotOptimize(block.data());
    }

    if (block == input_block) {
        state.SkipWithError("TODO 1: call the model inside the timed loop");
        return;
    }

    // Each repetition has to time exactly one call. If Google Benchmark picked
    // the number of iterations itself, every repetition is already an average,
    // and an average cannot show a worst case.
    if (state.iterations() != 1) {
        state.SkipWithError("TODO 3: time one single call per repetition");
    }
}

// The n-th percentile of the times Google Benchmark collected.
// @times: one entry per repetition, in seconds, in the order they were measured
// @fraction: 0.95 returns the time that 95 percent of the calls stayed under
double percentile(const std::vector<double>& times, double fraction) {
    // ---- TODO 2 ------------------------------------------------------------
    // Sort a copy of `times` and return the entry at `fraction` of the way
    // through it.
    // ------------------------------------------------------------------------
    std::vector<double> sorted = std::nfill(times.size(), 0.0);
    const auto index = 0.0;

    return sorted[index];
}

// Says once that TODO 2 is still open: a percentile of 0 next to a mean of
// several milliseconds cannot be a real measurement.
double checked_percentile(const std::vector<double>& times, double fraction) {
    const double value = percentile(times, fraction);

    static bool reported = false;
    if (value == 0.0 && !times.empty() && !reported) {
        std::fprintf(stderr, "TODO 2: implement percentile(), it still returns 0\n");
        reported = true;
    }
    return value;
}

}  // namespace

// ---- TODO 3 ------------------------------------------------------------------
// Left alone, Google Benchmark runs the body as often as it likes and reports
// the average — which is the one number that cannot show a worst case. Make
// every repetition time a single call, and measure k_repetitions of them.
// Hint: ->Iterations(...) and ->Repetitions(...), like ->Unit() below.
// ------------------------------------------------------------------------------
BENCHMARK(forward_pass)
    ->Unit(benchmark::kMillisecond)
    ->ComputeStatistics("p95",
                        [](const std::vector<double>& t) { return checked_percentile(t, 0.95); })
    ->ComputeStatistics("p99",
                        [](const std::vector<double>& t) { return checked_percentile(t, 0.99); })
    ->ComputeStatistics("max",
                        [](const std::vector<double>& t) {
                            return *std::max_element(t.begin(), t.end());
                        })
    ->ReportAggregatesOnly(true);

int main(int argc, char** argv) {
    try {
        engine();  // Load the model before anything is timed
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    // How much time one host block of audio is worth. The model needs 2048
    // samples before it can run at all, but the deadline belongs to the callback
    // it runs in — so the smaller the host block, the less time there is.
    std::printf("Budget per callback at %.0f Hz:\n", k_model.m_sample_rate);
    for (const int host_block : {64, 128, 256, 512, 1024, 2048}) {
        std::printf("  %5d samples   %6.2f ms\n",
                    host_block,
                    1000.0 * host_block / k_model.m_sample_rate);
    }
    std::printf("\n");

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
