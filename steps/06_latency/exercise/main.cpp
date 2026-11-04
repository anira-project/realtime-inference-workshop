// Step 6 — Latency and the dry/wet mix
//
// Goal:   work out how late the processed signal is, report it, and delay the
//         dry signal by the same amount so the two can be mixed.
// Given:  the threaded processor from step 5, with a dry path added.
// Task:   fill in the two TODO banners.
// Check:  at 100% dry the output is the input, delayed by exactly the reported
//         latency — if the number is wrong, the check says so.
//
// It builds and runs as it is, and says which TODO is still open.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/helpers/host.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/ring_buffer.h"
#include "common/helpers/support.h"
#include "common/helpers/threadsafe_ring_buffer.h"

// Model settings taken from the export metadata.
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    size_t m_input_size = 2048;
    double m_sample_rate = 44100.0;
    size_t m_latency = 2224;  // The model's own delay: how much later its output answers its input
} k_model{};

constexpr size_t k_host_block_size = 512;  // What the host hands over per callback

// Your worst case after the warm-up, from the last lines of step 3, in ms.
constexpr double k_max_inference_ms = 0.0;  // <- fill out: your number from step 3

// The first calls are slow (step 3): made before the audio starts.
constexpr size_t k_warm_up_calls = 2;

namespace {

#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

// Step 5: your worst case in samples, rounded up to whole callbacks.
size_t rounded_max_inference_time() {
    const double max_inference_samples = k_max_inference_ms * k_model.m_sample_rate / 1000.0;
    return static_cast<size_t>(
               std::ceil(max_inference_samples / static_cast<double>(k_host_block_size))) *
           k_host_block_size;
}

// Step 5's processor, with a dry path and a mix control.
class LatencyProcessor {
public:
    explicit LatencyProcessor(LibTorchEngine& engine) : m_engine(engine) {}

    ~LatencyProcessor() { stop(); }

    // Step 5's prefill: how late the wet path comes out of m_output.
    size_t prefill_samples() const {
        return (k_model.m_input_size - k_host_block_size) + rounded_max_inference_time();
    }

    // ---- TODO 1 -------------------------------------------------------------
    // How late is the wet signal?
    // Three things delay it, and all are fixed:
    //   - the buffer adaptation: the model cannot run before a whole block has
    //     arrived, the host brings 512 samples at a time (step 2)
    //   - the time the worker is given: rounded_max_inference_time() (step 5)
    //   - the model itself answers late: k_model.m_latency, from the export
    // What the host is told has to be what the host gets, so this number also
    // decides how far the dry signal is delayed in prepare().
    // --------------------------------------------------------------------------
    size_t latency_samples() const {
        size_t latency = 0;  // <- fill out
        return latency;
    }

    // Allocate and initialize all processing buffers and the thread
    // before audio processing begins.
    // @max_block_size: the largest block process_block() can be given
    void prepare(size_t max_block_size) {
        stop();

        // Step 5: the input holds one model block, the output starts with the prefill.
        m_input = ThreadSafeRingBuffer(k_model.m_input_size);
        m_output = ThreadSafeRingBuffer(rounded_max_inference_time() + k_model.m_input_size);
        const std::vector<float> silence(std::max(latency_samples(), prefill_samples()), 0.0f);
        m_output.push(silence.data(), prefill_samples());

        // The dry path: the audio thread's own ring buffer, and scratch blocks.
        m_dry = RingBuffer(max_block_size + latency_samples());
        m_dry_block.assign(max_block_size, 0.0f);
        m_wet_block.assign(max_block_size, 0.0f);

        // ---- TODO 2 --------------------------------------------------------
        // Delay the dry signal.
        // The host aligns everything to latency_samples(). How far behind does
        // the dry signal have to start, so it lines up with the wet one?
        // The model's own delay is already inside what it returns; the dry
        // path has nothing like that, so it gets all of it.
        // ---------------------------------------------------------------------
        m_dry.push(silence.data(), 0);  // <- fill out

        // Step 5: the warm-up, before the audio starts.
        m_model_block.assign(k_model.m_input_size, 0.0f);
        for (size_t i = 0; i < k_warm_up_calls; ++i) {
            m_engine.process(m_model_block.data(), k_model.m_input_size);
        }
        m_engine.reset();

        m_unpushed = 0;
        m_debt = 0;

        m_running.store(true, std::memory_order_release);
        m_worker = std::thread([this] { worker(); });
    }

    void stop() {
        if (!m_worker.joinable()) { return; }
        m_running.store(false, std::memory_order_release);
        m_worker.join();
    }

    // @mix: 0 is dry only, 1 is wet only
    void set_mix(float mix) { m_mix = mix; }

    // The audio thread: samples in, samples out, nothing that can block.
    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
        // Step 5's audio thread, into m_wet_block instead of the host's buffer.
        for (size_t i = 0; i < num_samples; ++i) {
            if (!m_input.try_push(samples[i])) {
                m_unpushed += num_samples - i;
                break;
            }
        }
        const size_t zeros = std::min(m_unpushed, num_samples);
        std::fill_n(m_wet_block.data(), zeros, 0.0f);
        m_unpushed -= zeros;

        float late = 0.0f;
        while (m_debt > 0 && m_output.try_pop(late)) { --m_debt; }

        for (size_t i = zeros; i < num_samples; ++i) {
            if (!m_output.try_pop(m_wet_block[i])) {
                m_wet_block[i] = 0.0f;
                m_debt += 1;
            }
        }

        // The dry path: every sample goes in, and comes out as late as m_dry
        // was prefilled.
        m_dry.push(samples, num_samples);
        m_dry.pop(m_dry_block.data(), num_samples);

        // Both sides are equally late now, so this is a plain crossfade.
        const float dry_gain = 1.0f - m_mix;
        const float wet_gain = m_mix;
        for (size_t i = 0; i < num_samples; ++i) {
            samples[i] = dry_gain * m_dry_block[i] + wet_gain * m_wet_block[i];
        }
    }

private:
    // Step 5's worker thread.
    void worker() {
        while (m_running.load(std::memory_order_acquire)) {
            if (m_input.available() < k_model.m_input_size) { continue; }

            m_input.pop(m_model_block.data(), k_model.m_input_size);
            m_engine.process(m_model_block.data(), k_model.m_input_size);

            size_t pushed = 0;
            while (pushed < k_model.m_input_size && m_running.load(std::memory_order_acquire)) {
                if (m_output.try_push(m_model_block[pushed])) { ++pushed; }
            }
        }
    }

    LibTorchEngine& m_engine;
    ThreadSafeRingBuffer m_input{0};   // Written by the audio thread, read by the worker
    ThreadSafeRingBuffer m_output{0};  // Written by the worker, read by the audio thread
    RingBuffer m_dry{0};               // The audio thread's own
    std::vector<float> m_model_block;  // The worker's block, sized in prepare()
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::vector<float> m_dry_block;  // Scratch, sized in prepare()
    std::vector<float> m_wet_block;
    size_t m_unpushed = 0;  // Audio thread: samples that did not fit into m_input, owed as zeros
    size_t m_debt = 0;      // Audio thread: samples that went out as zeros, thrown away when they
                            // arrive
    float m_mix = 1.0f;
};

// Runs the whole signal through the processor at one mix setting.
std::vector<float> run(LatencyProcessor& processor, float mix) {
    processor.prepare(k_host_block_size);
    processor.set_mix(mix);

    std::vector<float> output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host_block_size,
        [&processor](float* samples, size_t num_samples) {
            processor.process_block(samples, num_samples);
        },
        k_model.m_sample_rate);

    processor.stop();
    return output;
}

bool silent(const std::vector<float>& signal) {
    return std::all_of(signal.begin(), signal.end(), [](float x) { return x == 0.0f; });
}

}  // namespace

int main() {
    LibTorchEngine engine;
    try {
        engine.load_model(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    if (k_max_inference_ms <= 0.0) {
        std::printf("TODO 1: put your worst case from step 3 into k_max_inference_ms.\n");
        return 1;
    }

    LatencyProcessor processor(engine);
    const size_t latency = processor.latency_samples();

    if (latency == 0) {
        std::printf("TODO 1: work out the latency the design introduces.\n");
        return 1;
    }

    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                latency,
                1000.0 * static_cast<double>(latency) / k_model.m_sample_rate,
                k_model.m_sample_rate);

    const std::vector<float> dry = run(processor, 0.0f);
    if (silent(dry)) {
        std::printf("At mix 0 the dry signal has to come through, but the output is silent.\n");
        return 1;
    }
    if (diff_with_latency(dry.data(), k_input_signal.data(), dry.size(), 0) <= k_tolerance) {
        std::printf(
            "TODO 2: the dry signal comes out undelayed, so it cannot line up with the wet one.\n");
        return 1;
    }

    const std::vector<float> wet = run(processor, 1.0f);
    if (silent(wet)) {
        std::printf("At mix 1 the wet signal has to come through, but the output is silent.\n");
        return 1;
    }

    // The dry path is the audio thread's own: exact, no dropouts.
    const bool dry_ok =
        report_line("dry, delayed by the latency",
                    diff_with_latency(dry.data(), k_input_signal.data(), dry.size(), latency));

    // The reference already carries the model's own delay, so the wet path
    // lines up with it after the rest of the reported latency. A busy machine
    // can make the worker late now and then: those callbacks are dropouts.
    if (latency < k_model.m_latency) {
        std::printf("\nFAILED: the reported latency is shorter than the model's own.\n");
        return 1;
    }
    std::printf("  wet, delayed by the latency:\n");
    const bool wet_ok = report_with_dropouts(wet.data(),
                                             k_target_output_signal.data(),
                                             wet.size(),
                                             latency - k_model.m_latency,
                                             k_host_block_size) == 0;

    if (dry_ok && wet_ok) {
        std::printf("\nOK: both paths are late by exactly the reported latency, so they mix.\n");
        return 0;
    }
    std::printf("\nFAILED: the reported latency does not match what comes out.\n");
    return 1;
}
