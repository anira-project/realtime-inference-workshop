// Step 5 — Inference on a worker thread, reference implementation
//
// Goal:   get the engine off the audio thread, and keep the audio thread free
//         of allocations and locks.
// Given:  ThreadSafeRingBuffer, step 2's ring buffer made safe between two
//         threads, and the fake host.
// Task:   fill in the three TODO banners.
// Check:  the model still produces the reference output, and the callback does
//         not allocate once.
//
// It builds and runs as it is, and says which TODO is still open.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <new>
#include <stdexcept>
#include <thread>
#include <vector>

#include "common/helpers/host.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/threadsafe_ring_buffer.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

// Model settings taken from the export metadata.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_sample_rate: the sample rate the model was trained for
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    size_t m_input_size = 2048;
    double m_sample_rate = 44100.0;
} k_model{};

constexpr size_t k_host_block_size = 512;  // What the host hands over per callback
constexpr size_t k_blocks_in_flight = 8;   // Model blocks waiting between the threads

// Counts what the audio thread allocates. Any number above zero here is a bug,
// whether or not it showed up as a glitch today.
namespace {
// Keep this flag thread-local: only allocations on the audio thread matter.
// The worker thread may allocate, so counting its allocations would not test
// the real-time constraint.
thread_local bool t_on_audio_thread = false;
std::atomic<long> g_allocations{0};
}  // namespace

void* operator new(size_t size) {
    if (t_on_audio_thread) { g_allocations.fetch_add(1, std::memory_order_relaxed); }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) { throw std::bad_alloc(); }
    return memory;
}

void operator delete(void* memory) noexcept {
    if (t_on_audio_thread) { g_allocations.fetch_add(1, std::memory_order_relaxed); }
    std::free(memory);
}

void operator delete(void* memory, size_t) noexcept {
    operator delete(memory);
}

namespace {

// The audio callback promises not to block. Marking it is what lets RTSan check
// everything underneath, see step 4.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

// Step 2's processor, on two threads: the callback only moves samples, the
// worker runs the engine. Two thread-safe ring buffers connect them:
//
//   audio thread:  host block -> m_input              m_output -> host block
//   worker thread:               m_input -> model -> m_output
//
// Each has one writer and one reader, on different threads.
class ThreadedProcessor {
public:
    explicit ThreadedProcessor(LibTorchEngine& engine) : m_engine(engine) {}

    ~ThreadedProcessor() { stop(); }

    // Everything that allocates: buffers, and the thread itself.
    // @max_block_size: the largest block the callback will be given
    void prepare(size_t max_block_size) {
        stop();

        // The worker can hand several blocks back at once, so the output side
        // has to hold what can be in flight.
        m_output = ThreadSafeRingBuffer(max_block_size + k_blocks_in_flight * k_model.m_input_size);
        m_model_block.assign(k_model.m_input_size, 0.0f);
        m_engine.reset();
        m_received = 0;
        m_produced.clear();
        m_produced.reserve(k_signal_length);

        // ---- TODO 1 --------------------------------------------------------
        //   1. Size m_input. In step 2 the model ran right away, so one model
        //      block was enough. Now the worker can fall behind: room for
        //      k_blocks_in_flight model blocks, on top of one host block.
        //   2. Start the worker thread. Which function does it run?
        // --------------------------------------------------------------------
        const size_t input_capacity = max_block_size + k_blocks_in_flight * k_model.m_input_size; // <- solution

        if (input_capacity < max_block_size + k_model.m_input_size) {
            throw std::runtime_error("TODO 1: give m_input room for at least a host block and a model block");
        }
        m_input = ThreadSafeRingBuffer(input_capacity);

        m_running.store(true, std::memory_order_release);
        m_worker = std::thread([this] { worker(); }); // <- solution
    }

    // Finish what is still in flight, then stop the thread.
    void stop() {
        if (!m_worker.joinable()) { return; }
        m_running.store(false, std::memory_order_release);
        m_worker.join();
    }

    // The audio thread: no engine, no allocation, no lock. Only copies.
    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
        // ---- TODO 2 --------------------------------------------------------
        // The audio thread: step 2's process_block(), without the engine.
        //   1. push the host's samples into m_input, if there is room
        //   2. hand the host its samples from m_output, if they are ready
        //
        // Hint:
        //   - space() and available() never wait. No room means the worker is
        //     behind: drop the block, do not wait for it.
        // --------------------------------------------------------------------
        const bool has_room = m_input.space() >= num_samples; // <- solution

        if (has_room) {
            m_input.push(samples, num_samples); // <- solution
            m_received += num_samples;
        } else {
            m_dropped.fetch_add(1, std::memory_order_relaxed);  // The worker is behind: dropped
        }

        const bool is_ready = m_output.available() >= num_samples; // <- solution

        if (is_ready) {
            m_output.pop(samples, num_samples); // <- solution
        } else {
            std::fill_n(samples, num_samples, 0.0f);
        }
    }

    // Everything the model produced, in order — what the check reads.
    const std::vector<float>& produced() const { return m_produced; }

    // Samples the audio thread handed over to the worker.
    size_t received() const { return m_received; }

    // True once prepare() has started the worker thread.
    bool worker_started() const { return m_worker.joinable(); }

private:
    // The worker thread: the engine lives here, and everything the engine does
    // — allocating, locking, growing arenas — is allowed on this side.
    void worker() {
        while (m_running.load(std::memory_order_acquire)) {
            // ---- TODO 3 ----------------------------------------------------
            // The worker thread: the other half of step 2's process_block().
            //   1. wait until m_input holds a whole model block
            //   2. take it out and run the engine on it
            //   3. push the result into m_output
            // Nothing here is real-time: sleeping, waiting and allocating are
            // all allowed on this side.
            // ------------------------------------------------------------------
            const bool has_block = m_input.available() >= k_model.m_input_size; // <- solution

            if (!has_block) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }

            m_input.pop(m_model_block.data(), k_model.m_input_size); // <- solution
            m_engine.process(m_model_block.data(), k_model.m_input_size); // <- solution
            m_produced.insert(m_produced.end(), m_model_block.begin(), m_model_block.end());

            // Wait for room, but keep watching the stop flag: once the host
            // stops calling back, nobody makes room, and stop() would wait forever.
            while (m_running.load(std::memory_order_acquire) &&
                   m_output.space() < k_model.m_input_size) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            if (m_output.space() >= k_model.m_input_size) {
                m_output.push(m_model_block.data(), k_model.m_input_size); // <- solution
            }
        }

        // Drain what the host pushed just before the end.
        while (m_input.available() >= k_model.m_input_size) {
            m_input.pop(m_model_block.data(), k_model.m_input_size);
            m_engine.process(m_model_block.data(), k_model.m_input_size);
            m_produced.insert(m_produced.end(), m_model_block.begin(), m_model_block.end());
        }
    }

    LibTorchEngine& m_engine;
    ThreadSafeRingBuffer m_input{0};   // Written by the audio thread, read by the worker
    ThreadSafeRingBuffer m_output{0};  // Written by the worker, read by the audio thread
    std::vector<float> m_model_block;  // The worker's block, sized in prepare()
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<long> m_dropped{0};  // Host blocks the worker had no room for
    size_t m_received = 0;           // Written on the audio thread only
    std::vector<float> m_produced;   // Written on the worker thread only
};

}  // namespace

int main() {
    LibTorchEngine engine;
    try {
        engine.load_model(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    ThreadedProcessor processor(engine);
    try {
        processor.prepare(k_host_block_size);
    } catch (const std::runtime_error& error) {
        std::printf("%s\n", error.what());
        return 1;
    }
    if (!processor.worker_started()) {
        std::printf("TODO 1: start the worker thread.\n");
        return 1;
    }

    // The host now runs in real time: the worker has to keep up with a block of
    // audio every 11.6 ms, rather than with a loop going as fast as it can.
    g_allocations.store(0, std::memory_order_relaxed);

    // Only what happens inside the callback counts as the audio thread.
    const std::vector<float> host_output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host_block_size,
        [&processor](float* samples, size_t num_samples) {
            t_on_audio_thread = true;
            processor.process_block(samples, num_samples);
            t_on_audio_thread = false;
        },
        k_model.m_sample_rate);

    processor.stop();

    const long allocations = g_allocations.load(std::memory_order_relaxed);
    const std::vector<float>& produced = processor.produced();

    if (produced.empty()) {
        std::printf(processor.received() == 0
                        ? "TODO 2: the audio thread never handed a block over.\n"
                        : "TODO 3: the worker never picked a block up.\n");
        return 1;
    }

    // The worker ran the model, but nothing came back to the host.
    if (std::all_of(host_output.begin(), host_output.end(), [](float x) { return x == 0.0f; })) {
        std::printf("TODO 2 or 3: the host only got silence. Does the worker push its blocks\n"
                    "into m_output, and does the audio thread pop them?\n");
        return 1;
    }

    std::printf("  %-28s %ld\n", "allocations on the audio thread", allocations);
    std::printf("  %-28s %zu of %zu samples\n",
                "the model produced",
                produced.size(),
                k_input_signal.size());

    const float diff =
        max_abs_diff(produced.data(), k_target_output_signal.data(), produced.size());
    std::printf("  %-28s %.3g\n\n", "max abs diff vs. reference", diff);

    if (allocations > 0) {
        std::printf("FAILED: the audio thread allocated %ld times.\n", allocations);
        return 1;
    }
    if (diff > k_tolerance) {
        std::printf("FAILED: the output drifted from the reference.\n");
        return 1;
    }
    std::printf("OK: inference ran on the worker thread, the audio thread only moved samples.\n");
    return 0;
}
