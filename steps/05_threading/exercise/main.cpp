// Step 5 — Inference on a worker thread
//
// Goal:   get the engine off the audio thread, keep the audio thread free of
//         allocations and locks, and keep the latency fixed.
// Given:  ThreadSafeRingBuffer, step 2's ring buffer made safe between two
//         threads, and the fake host.
// Task:   fill in the three TODO banners.
// Check:  the host gets the model's stream back, exactly the prefill later,
//         and the callback does not allocate once.
//
// It builds and runs as it is, and says which TODO is still open.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <thread>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/helpers/host.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/support.h"
#include "common/helpers/threadsafe_ring_buffer.h"

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

// Your worst case after the warm-up, from the last lines of step 3, in ms.
constexpr double k_max_inference_ms = 0.0;  // <- fill out: your number from step 3

// The first calls are slow (step 3): made before the audio starts.
constexpr size_t k_warm_up_calls = 2;

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
// Each has one writer and one reader, on different threads. m_output starts
// with the prefill: as many zeros as the host is late, so that the worker has
// time and the host always finds something to take.
class ThreadedProcessor {
public:
    explicit ThreadedProcessor(LibTorchEngine& engine) : m_engine(engine) {}

    ~ThreadedProcessor() { stop(); }

    // Everything that allocates: buffers, the warm-up, and the thread itself.
    // @max_block_size: the largest block the callback will be given
    void prepare(size_t max_block_size) {
        stop();

        if (k_max_inference_ms <= 0.0) {
            throw std::runtime_error(
                "TODO 1: put your worst case from step 3 into k_max_inference_ms");
        }

        // ---- TODO 1 --------------------------------------------------------
        //   1. rounded_max_inference_time: k_max_inference_ms in samples,
        //      rounded up to whole callbacks of max_block_size samples
        //   2. the prefill: the buffer adaptation from step 2 plus the time
        //      the worker is given
        //   3. the size of m_output: the worker's time plus one model block
        //   4. start the worker thread. Which function does it run?
        // --------------------------------------------------------------------
        const double max_inference_samples = k_max_inference_ms * k_model.m_sample_rate / 1000.0;
        const size_t rounded_max_inference_time = 0;  // <- 1. fill out
        m_prefill = 0;                                // <- 2. fill out
        const size_t output_capacity = 0;             // <- 3. fill out

        if (rounded_max_inference_time == 0 || rounded_max_inference_time % max_block_size != 0 ||
            static_cast<double>(rounded_max_inference_time) < max_inference_samples) {
            throw std::runtime_error(
                "TODO 1: round k_max_inference_ms up to whole callbacks, in samples");
        }
        if (m_prefill == 0) { throw std::runtime_error("TODO 1: work out the prefill"); }
        if (output_capacity < m_prefill + max_block_size) {
            throw std::runtime_error(
                "TODO 1: give m_output room for the prefill and a callback more");
        }

        // Given: the input holds one model block, as in step 2. The output
        // starts with the prefill.
        m_input = ThreadSafeRingBuffer(k_model.m_input_size);
        m_output = ThreadSafeRingBuffer(output_capacity);
        const std::vector<float> silence(m_prefill, 0.0f);
        m_output.push(silence.data(), m_prefill);

        // Given: the warm-up. The first calls are slow (step 3), so they are
        // made here, before the audio starts; then the model state is cleared.
        m_model_block.assign(k_model.m_input_size, 0.0f);
        for (size_t i = 0; i < k_warm_up_calls; ++i) {
            m_engine.process(m_model_block.data(), k_model.m_input_size);
        }
        m_engine.reset();

        m_unpushed = 0;
        m_debt = 0;

        m_running.store(true, std::memory_order_release);
        // m_worker = std::thread([this] { ?(); });  // <- 4. fill out
    }

    // Stop the worker thread.
    void stop() {
        if (!m_worker.joinable()) { return; }
        m_running.store(false, std::memory_order_release);
        m_worker.join();
    }

    // The audio thread: no engine, no allocation, no lock, no waiting. Only copies.
    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
        // ---- TODO 2 --------------------------------------------------------
        // The audio thread: step 2's process_block(), without the engine.
        //   1. push the host's samples into m_input, one by one. If one does
        //      not fit, the worker is a whole block behind: count it and the
        //      rest in m_unpushed, and stop
        //   2. (given) those never reach the model: the host gets as many
        //      zeros instead, and nothing is taken out of m_output for them
        //   3. take the rest from m_output, one by one. If one is not there
        //      yet, or something is still owed, the worker is late: it and the
        //      rest of this callback go out as zeros, and all of them are
        //      owed: m_debt
        //   4. pay the debt first: what is owed came too late, its place
        //      already went out as a zero. Take it out and throw it away
        //
        // Hint: try_push() and try_pop() never wait, they say whether it worked.
        // --------------------------------------------------------------------
        for (size_t i = 0; i < num_samples; ++i) {
            const bool pushed = false;  // <- 1. fill out
            if (!pushed) {
                m_unpushed += 0;  // <- 1. fill out
                break;
            }
        }

        const size_t zeros = std::min(m_unpushed, num_samples);
        std::fill_n(samples, zeros, 0.0f);
        m_unpushed -= zeros;

        [[maybe_unused]] float late = 0.0f;
        while (m_debt > 0 && false) { --m_debt; }  // <- 4. fill out

        for (size_t i = zeros; i < num_samples; ++i) {
            const bool popped = false;  // <- 3. fill out
            if (!popped) {
                // Not the next one either: while anything is owed, or while
                // the worker is pushing right now, what comes next would land
                // one place too early.
                std::fill_n(samples + i, num_samples - i, 0.0f);
                m_debt += 0;  // <- 3. fill out
                break;
            }
        }
    }

    // How late the host hears the model's stream: the prefill, in samples.
    size_t prefill_samples() const { return m_prefill; }

    // True once prepare() has started the worker thread.
    bool worker_started() const { return m_worker.joinable(); }

private:
    // The worker thread: the engine lives here, and everything the engine does
    // — allocating, locking, growing arenas — is allowed on this side.
    void worker() {
        while (m_running.load(std::memory_order_acquire)) {
            // ---- TODO 3 ----------------------------------------------------
            // The worker thread: the other half of step 2's process_block().
            //   1. check whether m_input holds a whole model block. If not,
            //      check again: no sleep, the result is needed soon
            //   2. take it out and run the engine on it
            //   3. push the result into m_output, sample by sample. If
            //      m_output is full, try again until there is room, but keep
            //      watching m_running: once the host stops calling, nobody
            //      makes room, and stop() would wait forever
            // ------------------------------------------------------------------
            const bool has_block = false;  // <- 1. fill out

            if (!has_block) { continue; }

            // m_input.pop(?, ?);  // <- 2. fill out
            // m_engine.process(?, ?);  // <- 2. fill out

            size_t pushed = 0;
            while (pushed < k_model.m_input_size && m_running.load(std::memory_order_acquire)) {
                if (false) { ++pushed; }  // <- 3. fill out
            }
        }
    }

    LibTorchEngine& m_engine;
    ThreadSafeRingBuffer m_input{0};   // Written by the audio thread, read by the worker
    ThreadSafeRingBuffer m_output{0};  // Written by the worker, read by the audio thread
    std::vector<float> m_model_block;  // The worker's block, sized in prepare()
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    size_t m_prefill = 0;   // Set in prepare()
    size_t m_unpushed = 0;  // Audio thread: samples that did not fit into m_input, owed as zeros
    size_t m_debt = 0;      // Audio thread: samples that went out as zeros, thrown away when they
                            // arrive
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
    const size_t prefill = processor.prefill_samples();

    // Nothing came back to the host.
    if (std::all_of(host_output.begin(), host_output.end(), [](float x) { return x == 0.0f; })) {
        std::printf(
            "TODO 2 or 3: the host only got silence. Does the audio thread push into m_input,\n"
            "does the worker push its blocks into m_output, and does the audio thread pop them?\n");
        return 1;
    }

    std::printf("  %-28s %zu samples (%.1f ms)\n",
                "prefill",
                prefill,
                1000.0 * static_cast<double>(prefill) / k_model.m_sample_rate);
    std::printf("  %-28s %ld\n", "allocations on the audio thread", allocations);

    if (allocations > 0) {
        std::printf("FAILED: the audio thread allocated %ld times.\n", allocations);
        return 1;
    }

    // What the host heard: the prefill in silence, then the model's stream,
    // apart from callbacks where the worker was late and zeros went out.
    const int result = report_with_dropouts(host_output.data(),
                                            k_target_output_signal.data(),
                                            host_output.size(),
                                            prefill,
                                            k_host_block_size);
    if (result != 0) {
        std::printf(
            "Dropouts or wrong samples mean the worker was late: raise k_max_inference_ms to\n"
            "the worst case of a few step 3 runs. If it is already that, the machine is busy.\n");
        return result;
    }
    std::printf(
        "OK: inference ran on the worker thread, the host got the model's stream back\n"
        "%zu samples late.\n",
        prefill);
    return 0;
}
