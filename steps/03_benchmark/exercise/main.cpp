// Step 3 — Benchmarking the forward pass
//
// Goal:   find out how long one forward pass takes, and how long it takes in
//         the worst case.
// Given:  Google Benchmark, and the engine from step 1.
// You do: fill in the three TODO banners.
// Check:  the numbers, against the budget a host block size gives you.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

#include "common/libtorch_engine.h"
#include "common/test_signal.h"

// What the export fixed, from its metadata.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_sample_rate: the sample rate the model was trained for
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 48000.0;
} k_model{};

// One forward pass is measured this many times. Each repetition is a single
// call, so the statistics below are over single calls — which is what a worst
// case means here.
constexpr int k_repetitions = 200;

namespace {

// Loaded once: loading is slow and has nothing to do with what we measure.
LibTorchEngine& engine() {
    static LibTorchEngine instance(k_model.m_path);
    return instance;
}

// One forward pass per iteration, on real audio.
void forward_pass(benchmark::State& state) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> block(k_input_signal.begin(), k_input_signal.begin() + block_size);

    const std::vector<float> untouched(block);

    for (auto _ : state) {
        // ---- TODO 1 --------------------------------------------------------
        // Run one forward pass on `block`. Everything inside this loop is what
        // gets timed, so nothing else belongs in here.
        // --------------------------------------------------------------------

        benchmark::DoNotOptimize(block.data());
    }

    if (block == untouched) { state.SkipWithError("TODO 1: run the model inside the timed loop"); }
}

// The n-th percentile of the per-call times Google Benchmark collected.
// @times: one entry per repetition, in seconds, unsorted
// @fraction: 0.95 for p95
double percentile(const std::vector<double>& times, double fraction) {
    // ---- TODO 2 ------------------------------------------------------------
    // Return the value `fraction` of the way through the sorted times. A p99
    // that reads 0 ms means this is still returning 0.
    // --------------------------------------------------------------------------
    return 0.0;
}

}  // namespace

// ---- TODO 3 ------------------------------------------------------------------
// Google Benchmark decides on its own how many calls to average over, and an
// average hides exactly what we are after. Make every repetition time one
// single forward pass, and take k_repetitions of them.
// Hint: ->Iterations(...) and ->Repetitions(...).
// --------------------------------------------------------------------------------
BENCHMARK(forward_pass)
    ->Unit(benchmark::kMillisecond)
    ->ComputeStatistics("p95", [](const std::vector<double>& t) { return percentile(t, 0.95); })
    ->ComputeStatistics("p99", [](const std::vector<double>& t) { return percentile(t, 0.99); })
    ->ComputeStatistics("max",
                        [](const std::vector<double>& t) {
                            return *std::max_element(t.begin(), t.end());
                        })
    ->ReportAggregatesOnly(true);

int main(int argc, char** argv) {
    try {
        engine();  // Load the model before timing anything
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    // What one host block of audio is worth in time. The model needs a whole
    // 2048-sample block before it can run, but the deadline belongs to the
    // callback the host is in — so the smaller the host block, the harder it is.
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
