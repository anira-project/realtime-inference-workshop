// Step 4 — Benchmarking the forward pass
//
// Goal:   find out how long one forward pass takes on your machine, and how long
//         it takes in the best and worst case, on ONNX Runtime and on LibTorch.
// Given:  Google Benchmark, the engine from step 1, and LibTorch to compare it with.
// Task:   none, this step is a demonstration. Build it, run it, read the numbers.
// Check:  the numbers, against the real-time threshold of a 512-sample callback,
//         and your worst case on ONNX Runtime: the number steps 6 and 7 need.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/assets/test_signal.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/onnx_engine.h"

// Model settings taken from the export metadata.
// @m_onnx_path: the ONNX model, state in and out, set via CMake
// @m_torchscript_path: the LibTorch model, state inside the model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_state_size: floats in the ONNX model's state
// @m_sample_rate: the sample rate the model was trained for
constexpr struct {
    const char* m_onnx_path = WORKSHOP_ONNX_MODEL_PATH;
    const char* m_torchscript_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    size_t m_state_size = 63488;
    double m_sample_rate = 44100.0;
} k_model{};

// What the host hands over per callback, as in every step of the first part.
constexpr size_t k_host_block_size = 512;

// How many times the forward pass is measured. Each repetition times one single
// call, so the statistics over these are statistics over real calls.
constexpr int k_repetitions = 200;

// The first calls are a warm-up: the engines set themselves up while they run the
// model, TorchScript most of all. A plugin makes them before the audio starts, so
// they do not count towards the worst case.
constexpr size_t k_warm_up_calls = 2;

// How many calls are drawn, how wide the slowest one is, and how many of the
// slowest are listed.
constexpr size_t k_plotted_calls = 10;
constexpr int k_plot_width = 40;
constexpr size_t k_listed_calls = 5;

namespace {

// The models are loaded once. Loading is slow and is not what we measure.
OnnxEngine& onnx() {
    static OnnxEngine instance(k_model.m_onnx_path);
    return instance;
}

LibTorchEngine& libtorch() {
    static LibTorchEngine instance;
    if (!instance.is_model_loaded()) { instance.load_model(k_model.m_torchscript_path); }
    return instance;
}

// What Google Benchmark measures. Everything inside the loops below is measured.
void forward_pass_onnx(benchmark::State& state) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> block(k_input_signal.begin(), k_input_signal.begin() + block_size);
    std::vector<float> model_state(k_model.m_state_size, 0.0f);
    // Built up front, so the loop measures the engine and not these two lists.
    const std::vector<std::reference_wrapper<std::vector<float>>> buffers{block, model_state};
    const std::vector<size_t> sizes{block_size, k_model.m_state_size};

    for (auto _ : state) {
        onnx().process(buffers, sizes);
        benchmark::DoNotOptimize(block.data());
    }
}

void forward_pass_libtorch(benchmark::State& state) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> block(k_input_signal.begin(), k_input_signal.begin() + block_size);

    for (auto _ : state) {
        libtorch().process(block.data(), block_size);
        benchmark::DoNotOptimize(block.data());
    }
}

// Prints Google Benchmark's summary as usual, and keeps the time of every single
// call per benchmark, in the order they were measured.
class CallTimes : public benchmark::ConsoleReporter {
public:
    void ReportRuns(const std::vector<Run>& runs) override {
        std::vector<Run> summary;
        for (const Run& run : runs) {
            if (run.run_type == Run::RT_Iteration) {
                m_times[run.run_name.function_name].push_back(run.GetAdjustedRealTime());
            } else {
                summary.push_back(run);
            }
        }
        ConsoleReporter::ReportRuns(summary);
    }

    std::map<std::string, std::vector<double>> m_times;
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
void one_call_per_repetition(benchmark::internal::Benchmark* benchmark) {
    benchmark->Unit(benchmark::kMillisecond)
        ->Iterations(1)
        ->Repetitions(k_repetitions)
        ->ComputeStatistics("min", fastest)
        ->ComputeStatistics("p95", [](const std::vector<double>& t) { return percentile(t, 0.95); })
        ->ComputeStatistics("p99", [](const std::vector<double>& t) { return percentile(t, 0.99); })
        ->ComputeStatistics("max", slowest);
}

BENCHMARK(forward_pass_onnx)->Apply(one_call_per_repetition);
BENCHMARK(forward_pass_libtorch)->Apply(one_call_per_repetition);

// The first calls and the slowest calls of one engine.
// Returns its worst case after the warm-up, in ms.
double print_calls(const char* name, const std::vector<double>& times) {
    std::printf("\n%s, the first %zu of %zu calls:\n", name, k_plotted_calls, times.size());
    for (size_t i = 0; i < k_plotted_calls; ++i) {
        const int width = static_cast<int>(k_plot_width * times[i] / slowest(times) + 0.5);
        std::printf("  call %2zu  %-*s %6.2f ms\n",
                    i + 1,
                    k_plot_width,
                    std::string(static_cast<size_t>(width), '#').c_str(),
                    times[i]);
    }

    std::vector<size_t> calls_by_time(times.size());
    std::iota(calls_by_time.begin(), calls_by_time.end(), size_t{0});
    std::sort(calls_by_time.begin(), calls_by_time.end(), [&times](size_t a, size_t b) {
        return times[a] > times[b];
    });

    std::printf("\n%s, the %zu slowest calls:\n", name, k_listed_calls);
    for (size_t i = 0; i < k_listed_calls; ++i) {
        std::printf("  call %3zu  %6.2f ms\n", calls_by_time[i] + 1, times[calls_by_time[i]]);
    }

    return *std::max_element(times.begin() + static_cast<long>(k_warm_up_calls), times.end());
}

int main(int argc, char** argv) {
    try {
        onnx();  // Load the models before anything is timed
        libtorch();
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    // How much time a block of audio is worth. The model needs 2048 samples
    // before it can run at all, but the deadline belongs to the callback it runs
    // in, and the host's callback is only 512 samples long.
    std::printf("Real-time threshold at %.0f Hz:\n", k_model.m_sample_rate);
    std::printf("  model block  %5d samples   %6.2f ms\n",
                k_model.m_input_size,
                1000.0 * k_model.m_input_size / k_model.m_sample_rate);
    std::printf("  callback     %5zu samples   %6.2f ms\n\n",
                k_host_block_size,
                1000.0 * static_cast<double>(k_host_block_size) / k_model.m_sample_rate);

    benchmark::Initialize(&argc, argv);
    CallTimes calls;
    benchmark::RunSpecifiedBenchmarks(&calls);
    benchmark::Shutdown();

    const std::vector<double>& onnx_times = calls.m_times["forward_pass_onnx"];
    const std::vector<double>& libtorch_times = calls.m_times["forward_pass_libtorch"];

    const double max_inference_ms = print_calls("ONNX Runtime", onnx_times);
    const double libtorch_max_ms = print_calls("LibTorch", libtorch_times);

    std::printf("\nThe two engines, after the warm-up:\n");
    std::printf("               fastest   slowest\n");
    std::printf("  ONNX Runtime %6.2f ms %6.2f ms\n", fastest(onnx_times), max_inference_ms);
    std::printf("  LibTorch     %6.2f ms %6.2f ms\n", fastest(libtorch_times), libtorch_max_ms);

    // Your worst case after the warm-up, in samples: the time the worker in
    // step 6 has to be given, rounded up to whole host callbacks, because the
    // audio thread only picks results up once per callback.
    const double max_inference_samples = max_inference_ms * k_model.m_sample_rate / 1000.0;
    const auto rounded_max_inference_time =
        static_cast<size_t>(
            std::ceil(max_inference_samples / static_cast<double>(k_host_block_size))) *
        k_host_block_size;

    std::printf("\nYour worst case on ONNX Runtime after the warm-up (calls %zu to %zu):\n",
                k_warm_up_calls + 1,
                onnx_times.size());
    std::printf("  max inference time           %6.2f ms = %.0f samples\n",
                max_inference_ms,
                max_inference_samples);
    std::printf("  rounded_max_inference_time   %zu samples (whole %zu-sample callbacks)\n",
                rounded_max_inference_time,
                k_host_block_size);
    std::printf("\nPut %.2f into k_max_inference_ms in steps 6 and 7.\n", max_inference_ms);
    return 0;
}
