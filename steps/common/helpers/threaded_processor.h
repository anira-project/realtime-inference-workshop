// The processor from steps 6 and 7, given complete — this is what the plugin in
// step 8 runs.
//
// Goal: inference on a worker thread, lock-free ring buffers, a dry path delayed
//       to match, and one latency number the host can be told. Built for one
//       host block size, 512 samples: the prefill and the latency are only
//       right for that one.
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>

#include "common/helpers/ring_buffer.h"
#include "common/helpers/threadsafe_ring_buffer.h"

// Model settings taken from the export metadata.
constexpr struct {
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
    double m_sample_rate = 44100.0;
    size_t m_latency = 2224;  // The model's own delay: how much later its output answers its input
} k_model{};

// The only host block size this processor is built for.
constexpr size_t k_host_block_size = 512;

// Your worst case after the warm-up, from the last lines of step 4, in ms.
constexpr double k_max_inference_ms = 15.3;

// The first calls are slow (step 4): made before the audio starts.
constexpr size_t k_warm_up_calls = 2;

// What an audio callback promises; see step 5. RTSan checks everything below a
// function marked with it.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

// Step 7's processor: step 6's hand-over between the audio thread and a worker,
// with a dry path and a mix control.
// @Engine: anything with the process({audio, state}, {sizes}) of OnnxEngine. The
//          state is carried here, one per processor.
template <typename Engine>
class LatencyProcessor {
public:
    explicit LatencyProcessor(Engine& engine) : m_engine(engine) {}

    ~LatencyProcessor() { stop(); }

    // Whether prepare() can be called with this host block size.
    static constexpr bool supports_block_size(size_t block_size) {
        return block_size == k_host_block_size;
    }

    // Your worst case in samples, rounded up to whole callbacks: the worker's time.
    static size_t rounded_max_inference_time() {
        const double max_inference_samples = k_max_inference_ms * k_model.m_sample_rate / 1000.0;
        return static_cast<size_t>(
                   std::ceil(max_inference_samples / static_cast<double>(k_host_block_size))) *
               k_host_block_size;
    }

    // How late the wet path comes out of m_output: buffer adaptation plus the worker's time.
    static size_t prefill_samples() {
        return (k_model.m_input_size - k_host_block_size) + rounded_max_inference_time();
    }

    // What the host is told: the prefill, and the model's own delay on top.
    static size_t latency_samples() { return prefill_samples() + k_model.m_latency; }

    // Everything that allocates: buffers, the warm-up and the thread.
    // @max_block_size: the host block size; only k_host_block_size is supported
    void prepare(size_t max_block_size) {
        stop();
        if (!supports_block_size(max_block_size)) {
            throw std::invalid_argument(
                "LatencyProcessor: only a 512-sample host block is supported");
        }

        m_input = ThreadSafeRingBuffer(k_model.m_input_size);
        m_output = ThreadSafeRingBuffer(rounded_max_inference_time() + k_model.m_input_size);
        const std::vector<float> silence(latency_samples(), 0.0f);
        m_output.push(silence.data(), prefill_samples());

        // The dry path gets the whole latency: the model's own delay is already
        // inside what the wet path returns.
        m_dry = RingBuffer(max_block_size + latency_samples());
        m_dry.push(silence.data(), latency_samples());
        m_dry_block.assign(max_block_size, 0.0f);
        m_wet_block.assign(max_block_size, 0.0f);

        m_model_block.assign(k_model.m_input_size, 0.0f);
        m_state.assign(k_model.m_state_size, 0.0f);
        // Built once, so the worker does not allocate them on every block.
        m_buffers = {m_model_block, m_state};
        m_sizes = {k_model.m_input_size, k_model.m_state_size};

        for (size_t i = 0; i < k_warm_up_calls; ++i) {
            m_engine.process(m_buffers, m_sizes);
        }
        std::fill(m_state.begin(), m_state.end(), 0.0f);  // The warm-up is not part of the stream

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
    // @num_samples: at most the block size given to prepare()
    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
        // Into the worker's ring buffer, one sample at a time. What does not
        // fit never reaches the model: as many zeros go out instead.
        for (size_t i = 0; i < num_samples; ++i) {
            if (!m_input.try_push(samples[i])) {
                m_unpushed += num_samples - i;
                break;
            }
        }
        const size_t zeros = std::min(m_unpushed, num_samples);
        std::fill_n(m_wet_block.data(), zeros, 0.0f);
        m_unpushed -= zeros;

        // Samples that came too late already went out as zeros: thrown away
        // when they arrive, so the wet path stays exactly the prefill late.
        float late = 0.0f;
        while (m_debt > 0) {
            if (m_output.try_pop(late)) {
                --m_debt;
            } else {
                break;
            }
        }

        // The first sample that is not there yet ends the callback: it and the
        // rest go out as zeros and are owed. So does anything still owed: what
        // comes out next belongs to an earlier place, the worker is still
        // pushing it.
        for (size_t i = zeros; i < num_samples; ++i) {
            if (m_debt > 0 || !m_output.try_pop(m_wet_block[i])) {
                std::fill_n(m_wet_block.data() + i, num_samples - i, 0.0f);
                m_debt += num_samples - i;
                break;
            }
        }

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
    void worker() {
        while (m_running.load(std::memory_order_acquire)) {
            if (m_input.available() < k_model.m_input_size) { continue; }

            m_input.pop(m_model_block.data(), k_model.m_input_size);
            m_engine.process(m_buffers, m_sizes);

            // Wait for room, but keep watching the stop flag: the audio thread
            // may have stopped calling us, and then this would never return —
            // a lock-free ring buffer does not save you from a deadlock at shutdown.
            size_t pushed = 0;
            while (pushed < k_model.m_input_size && m_running.load(std::memory_order_acquire)) {
                if (m_output.try_push(m_model_block[pushed])) { ++pushed; }
            }
        }
    }

    Engine& m_engine;
    ThreadSafeRingBuffer m_input{0};   // Written by the audio thread, read by the worker
    ThreadSafeRingBuffer m_output{0};  // Written by the worker, read by the audio thread
    RingBuffer m_dry{0};               // The audio thread's own
    std::vector<float> m_model_block;  // The worker's block, sized in prepare()
    std::vector<float> m_state;        // The worker's state, sized in prepare()
    std::vector<std::reference_wrapper<std::vector<float>>> m_buffers;  // What the engine gets
    std::vector<size_t> m_sizes;
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::vector<float> m_dry_block;  // Scratch, sized in prepare()
    std::vector<float> m_wet_block;
    size_t m_unpushed = 0;  // Audio thread: samples that did not fit into m_input, owed as zeros
    size_t m_debt = 0;      // Audio thread: samples that went out as zeros, thrown away when they
                            // arrive
    float m_mix = 1.0f;
};
