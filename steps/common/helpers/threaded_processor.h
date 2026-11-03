// The processor from steps 5 and 6, given complete — this is what the plugin in
// step 8 runs.
//
// Goal: inference on a worker thread, lock-free ring buffers, a dry path delayed
//       to match, and one latency number the host can be told.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

#include "common/helpers/ring_buffer.h"
#include "common/helpers/threadsafe_ring_buffer.h"

// Model settings taken from the export metadata.
constexpr struct {
    size_t m_input_size = 2048;
    double m_sample_rate = 44100.0;
    size_t m_latency = 2224;  // The model's own delay: how much later its output answers its input
} k_model{};

constexpr size_t k_blocks_in_flight = 8;  // Model blocks waiting between the threads

// What an audio callback promises; see step 4. RTSan checks everything below a
// function marked with it.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

// Step 5's processor, with a dry path and a mix control.
// @Engine: anything with process(float*, size_t) and reset() — the engines from
//          step 1 and step 4 both fit, which is the point of that interface.
template <typename Engine>
class LatencyProcessor {
public:
    explicit LatencyProcessor(Engine& engine) : m_engine(engine) {}

    ~LatencyProcessor() { stop(); }

    // ---- TODO 1: how late is the wet signal? ---------------------------------
    // Three things delay it, and all are fixed:
    //   - the model cannot run before a whole block has arrived
    //   - the result is only picked up in a later callback, so the output side
    //     starts out primed with a block of silence
    //   - the model itself answers late: k_model.m_latency, from the export
    // What the host is told has to be what the host gets, so this number also
    // decides how far the dry signal is delayed.
    // --------------------------------------------------------------------------
    size_t latency_samples() const { return 2 * k_model.m_input_size + k_model.m_latency; }

    // Everything that allocates: buffers and the thread.
    // @max_block_size: the largest block process_block() will be given
    void prepare(size_t max_block_size) {
        stop();

        m_input = ThreadSafeRingBuffer(max_block_size + k_blocks_in_flight * k_model.m_input_size);
        m_output = ThreadSafeRingBuffer(max_block_size + k_blocks_in_flight * k_model.m_input_size);
        m_dry = RingBuffer(max_block_size + latency_samples());
        m_model_block.assign(k_model.m_input_size, 0.0f);
        m_engine.reset();
        m_dry_block.assign(max_block_size, 0.0f);
        m_wet_block.assign(max_block_size, 0.0f);
        m_wet_debt = 0;

        // Prime the wet path with one block of silence while the input is still
        // being collected, and one more so the worker has a full block of time
        // to answer. That is what makes the latency a fixed number instead of
        // whatever the worker happened to manage. The model's own delay is
        // already inside what it returns; the dry path gets all of it.
        m_silence.assign(std::max(latency_samples(), 2 * k_model.m_input_size), 0.0f);
        m_output.push(m_silence.data(), 2 * k_model.m_input_size);
        m_dry.push(m_silence.data(), latency_samples());

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
        // No room means the worker is behind. The block is dropped — a glitch,
        // but a bounded one. Waiting for room would be a missed deadline.
        if (m_input.space() >= num_samples) {
            m_input.push(samples, num_samples);
        } else {
            m_dropped.fetch_add(1, std::memory_order_relaxed);
        }
        m_dry.push(samples, num_samples);

        // A block that came too late already went out as silence. Drop it when
        // it arrives, so the wet path stays exactly latency_samples() late: a
        // late block is a glitch, not a shift for the rest of the stream.
        while (m_wet_debt > 0 && m_output.available() > 0) {
            const size_t n = std::min({m_wet_debt, m_output.available(), m_wet_block.size()});
            m_output.pop(m_wet_block.data(), n);
            m_wet_debt -= n;
        }
        const bool wet_ready = m_output.available() >= num_samples;

        // Both sides are equally late now, so this is a plain crossfade. If a
        // side has nothing ready, the other still has to come through.
        std::fill_n(samples, num_samples, 0.0f);

        if (m_dry.available() >= num_samples) {
            m_dry.pop(m_dry_block.data(), num_samples);
            for (size_t i = 0; i < num_samples; ++i) {
                samples[i] += (1.0f - m_mix) * m_dry_block[i];
            }
        }

        if (m_output.available() >= num_samples) {
            m_output.pop(m_wet_block.data(), num_samples);
            for (size_t i = 0; i < num_samples; ++i) { samples[i] += m_mix * m_wet_block[i]; }
        }
        // The wet side had nothing for this callback: its share is owed.
        if (!wet_ready) {
            m_wet_debt += num_samples;
        }
    }

private:
    void worker() {
        while (m_running.load(std::memory_order_acquire)) {
            if (m_input.available() < k_model.m_input_size) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            m_input.pop(m_model_block.data(), k_model.m_input_size);
            m_engine.process(m_model_block.data(), k_model.m_input_size);
            // Wait for room, but keep watching the stop flag: the audio thread
            // may have stopped calling us, and then this would never return —
            // a lock-free ring buffer does not save you from a deadlock at shutdown.
            while (m_running.load(std::memory_order_acquire) &&
                   m_output.space() < k_model.m_input_size) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            if (m_output.space() >= k_model.m_input_size) {
                m_output.push(m_model_block.data(), k_model.m_input_size);
            }
        }
        // Drain what the host pushed just before the end.
        while (m_input.available() >= k_model.m_input_size) {
            m_input.pop(m_model_block.data(), k_model.m_input_size);
            m_engine.process(m_model_block.data(), k_model.m_input_size);
        }
    }

    Engine& m_engine;
    ThreadSafeRingBuffer m_input{0};   // Written by the audio thread, read by the worker
    ThreadSafeRingBuffer m_output{0};  // Written by the worker, read by the audio thread
    RingBuffer m_dry{0};               // The audio thread's own
    std::vector<float> m_model_block;  // The worker's block, sized in prepare()
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<long> m_dropped{0};  // Host blocks the worker had no room for
    std::vector<float> m_silence;
    std::vector<float> m_dry_block;  // Scratch, sized in prepare()
    std::vector<float> m_wet_block;
    size_t m_wet_debt = 0;  // Wet samples whose slot already went out as silence
    float m_mix = 1.0f;
};
