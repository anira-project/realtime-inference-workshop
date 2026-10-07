// The processor from steps 6 and 7, given complete — this is what the plugin in
// step 8 runs.
//
// Goal: inference on a worker thread, lock-free hand-over, a dry path delayed
//       to match, and one latency number the host can be told.
#pragma once

#include <readerwriterqueue.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

#include "common/libtorch_engine.h"
#include "common/ring_buffer.h"

// Model settings taken from the export metadata.
constexpr struct {
    size_t m_input_size = 2048;
    double m_sample_rate = 48000.0;
} k_model{};

constexpr size_t k_queue_capacity = 8;  // Model blocks in flight between the threads

struct ModelBlock {
    std::array<float, 2048> m_samples{};
};

// What an audio callback promises; see step 5. RTSan checks everything below a
// function marked with it.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

// Step 6's processor, with a dry path and a mix control.
class LatencyProcessor {
public:
    explicit LatencyProcessor(LibTorchEngine& engine) : m_engine(engine) {}

    ~LatencyProcessor() { stop(); }

    // ---- TODO 1: how late is the wet signal? ---------------------------------
    // Two things delay it, and both are fixed by the design:
    //   - the model cannot run before a whole block has arrived
    //   - the result is only picked up in a later callback, so the output side
    //     starts out primed with a block of silence
    // What the host is told has to be what the host gets, so this number also
    // decides how far the dry signal is delayed.
    // --------------------------------------------------------------------------
    size_t latency_samples() const { return 2 * k_model.m_input_size; }

    // Everything that allocates: buffers, queue capacity, the thread.
    // @max_block_size: the largest block process_block() will be given
    void prepare(size_t max_block_size) {
        stop();

        m_input = RingBuffer(max_block_size + k_model.m_input_size);
        m_output = RingBuffer(max_block_size + k_queue_capacity * k_model.m_input_size);
        m_dry = RingBuffer(max_block_size + latency_samples());
        m_engine.reset();

        // Prime both paths with the same amount of silence: one block while the
        // input is still being collected, and one more so the worker has a full
        // block of time to answer. That is what makes the latency a fixed
        // number instead of whatever the worker happened to manage.
        m_silence.assign(latency_samples(), 0.0f);
        m_output.push(m_silence.data(), m_silence.size());
        m_dry.push(m_silence.data(), m_silence.size());

        m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);
        m_from_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);

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
        m_input.push(samples, num_samples);
        m_dry.push(samples, num_samples);

        while (m_input.available() >= k_model.m_input_size) {
            ModelBlock block;
            m_input.pop(block.m_samples.data(), k_model.m_input_size);
            if (!m_to_worker.try_enqueue(block)) { break; }
        }

        ModelBlock done;
        while (m_output.space() >= k_model.m_input_size && m_from_worker.try_dequeue(done)) {
            m_output.push(done.m_samples.data(), k_model.m_input_size);
        }

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
    }

    const std::vector<float>& produced() const { return m_produced; }

private:
    void worker() {
        ModelBlock block;
        while (m_running.load(std::memory_order_acquire)) {
            if (!m_to_worker.try_dequeue(block)) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            m_engine.process(block.m_samples.data(), k_model.m_input_size);
            m_produced.insert(m_produced.end(), block.m_samples.begin(), block.m_samples.end());
            while (!m_from_worker.try_enqueue(block)) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        while (m_to_worker.try_dequeue(block)) {
            m_engine.process(block.m_samples.data(), k_model.m_input_size);
            m_produced.insert(m_produced.end(), block.m_samples.begin(), block.m_samples.end());
        }
    }

    LibTorchEngine& m_engine;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    RingBuffer m_dry{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_to_worker{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_from_worker{0};
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::vector<float> m_produced;
    std::vector<float> m_silence;
    std::array<float, 4096> m_dry_block{};
    std::array<float, 4096> m_wet_block{};
    float m_mix = 1.0f;
};
