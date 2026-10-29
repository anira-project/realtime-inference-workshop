// Step 6 — Inference on a worker thread
//
// Goal:   get the engine off the audio thread, and keep the audio thread free
//         of allocations and locks.
// Given:  moodycamel's lock-free queues, the ring buffers, the fake host.
// Task:   fill in the three TODO banners.
// Check:  the model still produces the reference output, and the callback does
//         not allocate once.
//
// It builds and runs as it is, and says which TODO is still open.

#include <readerwriterqueue.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <new>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <vector>

#include "common/helpers/host.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/ring_buffer.h"
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
constexpr size_t k_queue_capacity = 8;     // Model blocks in flight between the threads

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

// One model block, by value: the queues copy these, so nothing is allocated and
// nobody has to agree on who owns what.
struct ModelBlock {
    std::array<float, 2048> m_samples{};
};

static_assert(std::tuple_size<decltype(ModelBlock::m_samples)>::value == k_model.m_input_size,
              "the block the queues carry has to be the block the model takes");

// The audio callback promises not to block. Marking it is what lets RTSan check
// everything underneath, see step 5.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

// The host's side and the model's side, now on two threads: the callback only
// moves samples, the worker runs the engine.
class ThreadedProcessor {
public:
    explicit ThreadedProcessor(LibTorchEngine& engine) : m_engine(engine) {}

    ~ThreadedProcessor() { stop(); }

    // Everything that allocates: buffers, queue capacity, and the thread itself.
    // @max_block_size: the largest block the callback will be given
    void prepare(size_t max_block_size) {
        stop();

        m_input = RingBuffer(max_block_size + k_model.m_input_size);
        // The worker can hand several blocks back at once, so the output side
        // has to hold what the queue can deliver in one go.
        m_output = RingBuffer(max_block_size + k_queue_capacity * k_model.m_input_size);
        m_engine.reset();
        m_produced.clear();
        m_produced.reserve(k_signal_length);

        // ---- TODO 1 --------------------------------------------------------
        //   1. Give both queues the capacity they need. The queues allocate
        //      their blocks when they are constructed, so try_enqueue() never
        //      has to grow them in the callback — it fails instead, which is
        //      what the audio thread needs.
        //   2. Start the worker thread. Which function does it run?
        // --------------------------------------------------------------------
        const size_t queue_capacity = 0; // <- 1. fill out

        if (queue_capacity == 0) {
            throw std::runtime_error("TODO 1: give the queues a capacity and start the worker");
        }
        m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(queue_capacity);
        m_from_worker = moodycamel::ReaderWriterQueue<ModelBlock>(queue_capacity);

        m_running.store(true, std::memory_order_release);
        // m_worker = std::thread([this] { ?(); }); // <- 2. fill out
    }

    // Finish what is still in flight, then stop the thread.
    void stop() {
        if (!m_worker.joinable()) { return; }
        m_running.store(false, std::memory_order_release);
        m_worker.join();
    }

    // The audio thread: no engine, no allocation, no lock. Only copies.
    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
         m_input.push(samples, num_samples);

        while (m_input.available() >= k_model.m_input_size) {
            ModelBlock block;
            m_input.pop(block.m_samples.data(), k_model.m_input_size);
            
        // ---- TODO 2 --------------------------------------------------------
        // The audio thread. 
        //   1. pass complete model blocks to the worker
        //   2. collect completed blocks
        //   3. push them into m_output — handing them to the host is given
        //
        // Hint:
        //   - try_enqueue() and try_dequeue() fail instead of blocking.
        // --------------------------------------------------------------------
            const bool hand_over_block = false; // <- 1. fill out

            if (!hand_over_block) {
                m_dropped.fetch_add(1, std::memory_order_relaxed);     // if it's not handed over, then it's dropped
            }
        }

        ModelBlock done;
        while (m_output.space() >= k_model.m_input_size) { // while there is space in the output buffer, try to collect completed blocks

            const bool got_a_block = false; // <- 2. fill out

            if (!got_a_block) { break; } 

            // m_output.push(?, ?); // <- 3. fill out
        }

        if (m_output.available() >= num_samples) {
            m_output.pop(samples, num_samples);
        } else {
            std::fill_n(samples, num_samples, 0.0f);
        }
    }

    // Everything the model produced, in order — what the check reads.
    const std::vector<float>& produced() const { return m_produced; }

    // Blocks the audio thread handed over that nobody picked up.
    size_t pending() const { return m_to_worker.size_approx(); }

    // True once prepare() has started the worker thread.
    bool worker_started() const { return m_worker.joinable(); }

private:
    // The worker thread: the engine lives here, and everything the engine does
    // — allocating, locking, growing arenas — is allowed on this side.
    void worker() {
        ModelBlock block;
        while (m_running.load(std::memory_order_acquire)) {
            // ---- TODO 3 ----------------------------------------------------
            // The worker thread. 
            //   1. Take a block out of m_to_worker 
            //   2. run the engine on it
            //   3. hand the block back through m_from_worker. 
            // Nothing here is real-time: sleeping, waiting and allocating are
            // all allowed on this side.
            // ------------------------------------------------------------------
            const bool got_a_block = false; // <- 1. fill out

            if (!got_a_block) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }

            // m_engine.process(?, ?); // <- 2. fill out
            m_produced.insert(m_produced.end(), block.m_samples.begin(), block.m_samples.end());

            while (m_running.load(std::memory_order_acquire)) {
                const bool hand_back_block = false; // <- 3. fill out
                if (hand_back_block) { break; }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }

        // Drain what the host pushed just before the end.
        while (m_to_worker.try_dequeue(block)) {
            m_engine.process(block.m_samples.data(), k_model.m_input_size);
            m_produced.insert(m_produced.end(), block.m_samples.begin(), block.m_samples.end());
        }
    }

    LibTorchEngine& m_engine;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_to_worker{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_from_worker{0};
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<long> m_dropped{0};  // Blocks the worker could not take in time
    std::vector<float> m_produced;  // Written on the worker thread only
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
        std::printf(processor.pending() == 0
                        ? "TODO 2: the audio thread never handed a block over.\n"
                        : "TODO 3: the worker never picked a block up.\n");
        return 1;
    }

    // The worker ran the model, but nothing came back to the host.
    if (std::all_of(host_output.begin(), host_output.end(), [](float x) { return x == 0.0f; })) {
        std::printf("TODO 2 or 3: the host only got silence. Does the worker hand its blocks\n"
                    "back, and does the audio thread collect them?\n");
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
