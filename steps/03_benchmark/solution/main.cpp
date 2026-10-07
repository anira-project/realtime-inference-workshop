// Step 3 — Benchmarking the forward pass, reference implementation
//
// Goal:  find out how long one forward pass takes, and how long it takes in the
//        worst case.
// Given: Google Benchmark, and the engine from step 1.
// Check: the numbers, against the budget a host block size gives you.

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

// What Google Benchmark times. Everything inside the loop below is measured.
void forward_pass(benchmark::State& state) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> block(k_input_signal.begin(), k_input_signal.begin() + block_size);

    for (auto _ : state) {
        // ---- TODO 1: call the model inside the timed loop ------------------
        engine().process(block.data(), block_size);

        benchmark::DoNotOptimize(block.data());
    }
}

// The n-th percentile of the times Google Benchmark collected.
// @times: one entry per repetition, in seconds, in the order they were measured
// @fraction: 0.95 returns the time that 95 percent of the calls stayed under
double percentile(const std::vector<double>& times, double fraction) {
    // ---- TODO 2: sort a copy and index into it ----------------------------
    std::vector<double> sorted(times);
    std::sort(sorted.begin(), sorted.end());
    const auto index = static_cast<size_t>(fraction * static_cast<double>(sorted.size() - 1) + 0.5);
    return sorted[index];
}

}  // namespace

// ---- TODO 3: one single call per repetition, k_repetitions of them -----------
// Left alone, Google Benchmark would run the body as often as it likes and
// report the average — the one number that cannot show a worst case.
// ------------------------------------------------------------------------------
BENCHMARK(forward_pass)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1)
    ->Repetitions(k_repetitions)
    ->ComputeStatistics("p95", [](const std::vector<double>& t) { return percentile(t, 0.95); })
    ->ComputeStatistics("p99", [](const std::vector<double>& t) { return percentile(t, 0.99); })
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
