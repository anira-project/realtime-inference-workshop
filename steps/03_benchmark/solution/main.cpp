// Step 3 — Benchmarking the forward pass
//
// Goal:   find out how long one forward pass takes on your machine, and how long
//         it takes in the best and worst case.
// Given:  Google Benchmark, and the engine from step 1.
// Task:   none, this step is a demonstration. Build it, run it, read the numbers.
// Check:  the numbers, against the real-time threshold of a host block size.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/helpers/libtorch_engine.h"
#include "common/assets/test_signal.h"

// Model settings taken from the export metadata.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_sample_rate: the sample rate the model was trained for
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 44100.0;
} k_model{};

// How many times the forward pass is measured. Each repetition times one single
// call, so the statistics over these are statistics over real calls.
constexpr int k_repetitions = 200;

// How many calls are drawn, how wide the slowest one is, and how many of the
// slowest are listed.
constexpr size_t k_plotted_calls = 10;
constexpr int k_plot_width = 40;
constexpr size_t k_listed_calls = 5;

namespace {

// The model is loaded once. Loading is slow and is not what we measure.
LibTorchEngine& engine() {
    static LibTorchEngine instance;
    if (!instance.is_model_loaded()) {
        instance.load_model(k_model.m_path);
    }
    return instance;
}

// What Google Benchmark measures. Everything inside the loop below is measured.
void forward_pass(benchmark::State& state) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> block(k_input_signal.begin(), k_input_signal.begin() + block_size);

    for (auto _ : state) {
        engine().process(block.data(), block_size);
        benchmark::DoNotOptimize(block.data());
    }
}

// Prints Google Benchmark's summary as usual, and keeps the time of every single
// call, in the order they were measured.
class CallTimes : public benchmark::ConsoleReporter {
public:
    void ReportRuns(const std::vector<Run>& runs) override {
        std::vector<Run> summary;
        for (const Run& run : runs) {
            if (run.run_type == Run::RT_Iteration) {
                m_times.push_back(run.GetAdjustedRealTime());
            } else {
                summary.push_back(run);
            }
        }
        ConsoleReporter::ReportRuns(summary);
    }

    std::vector<double> m_times;
};

// The n-th percentile of the times Google Benchmark collected.
// @times: one entry per repetition, in seconds, in the order they were measured
// @fraction: 0.95 returns the time that 95 percent of the calls stayed under
double percentile(const std::vector<double>& times, double fraction) {
    std::vector<double> sorted(times);
    std::sort(sorted.begin(), sorted.end());
    const auto index = static_cast<size_t>(fraction * static_cast<double>(sorted.size() - 1) + 0.5);
    return sorted[index];
}

double fastest(const std::vector<double>& times) {
    return *std::min_element(times.begin(), times.end());
}

double slowest(const std::vector<double>& times) {
    return *std::max_element(times.begin(), times.end());
}

}  // namespace

// One call per repetition: left alone, Google Benchmark runs the body as often
// as it likes and reports the average, which cannot show a worst case.
BENCHMARK(forward_pass)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1)
    ->Repetitions(k_repetitions)
    ->ComputeStatistics("min", fastest)
    ->ComputeStatistics("p95", [](const std::vector<double>& t) { return percentile(t, 0.95); })
    ->ComputeStatistics("p99", [](const std::vector<double>& t) { return percentile(t, 0.99); })
    ->ComputeStatistics("max", slowest);


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
    std::printf("Real-time threshold per callback at %.0f Hz:\n", k_model.m_sample_rate);
    for (const int host_block : {64, 128, 256, 512, 1024, 2048}) {
        std::printf("  %5d samples   %6.2f ms\n",
                    host_block,
                    1000.0 * host_block / k_model.m_sample_rate);
    }
    std::printf("\n");

    benchmark::Initialize(&argc, argv);
    CallTimes calls;
    benchmark::RunSpecifiedBenchmarks(&calls);
    benchmark::Shutdown();

    const std::vector<double>& times = calls.m_times;

    std::printf("\nThe first %zu of %zu calls:\n", k_plotted_calls, times.size());
    for (size_t i = 0; i < k_plotted_calls; ++i) {
        const int width = static_cast<int>(k_plot_width * times[i] / slowest(times) + 0.5);
        std::printf("  call %2zu  %-*s %6.2f ms\n",
                    i + 1, k_plot_width, std::string(static_cast<size_t>(width), '#').c_str(), times[i]);
    }

    std::vector<size_t> calls_by_time(times.size());
    std::iota(calls_by_time.begin(), calls_by_time.end(), size_t{0});
    std::sort(calls_by_time.begin(), calls_by_time.end(),
              [&times](size_t a, size_t b) { return times[a] > times[b]; });

    std::printf("\nThe %zu slowest calls:\n", k_listed_calls);
    for (size_t i = 0; i < k_listed_calls; ++i) {
        std::printf("  call %3zu  %6.2f ms\n", calls_by_time[i] + 1, times[calls_by_time[i]]);
    }
    return 0;
}
