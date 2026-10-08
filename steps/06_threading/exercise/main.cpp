// Step 6 — Inference on a worker thread
//
// Goal:   get the engine off the audio thread, and keep the audio thread free
//         of allocations and locks.
// Given:  moodycamel's lock-free queues, the ring buffers, the fake host.
// You do: fill in the three TODO banners.
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
#include <memory>
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
    double m_sample_rate = 48000.0;
} k_model{};

constexpr size_t k_host_block_size = 512;  // What the host hands over per callback
constexpr size_t k_queue_capacity = 8;     // Model blocks in flight between the threads

// Counts what the audio thread allocates. Any number above zero here is a bug,
// whether or not it showed up as a glitch today.
namespace {
// Thread-local on purpose: the worker thread is allowed to allocate as much as
// it likes, and counting its work here would say nothing.
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
        // Give both queues the capacity they need and start the worker thread.
        // The queues allocate their blocks when they are constructed, so
        // try_enqueue() never has to grow them in the callback — it fails
        // instead, which is what the audio thread needs.
        // The thread runs worker(), below.
        // --------------------------------------------------------------------
        const size_t queue_capacity = 0;

        if (queue_capacity == 0) {
            throw std::runtime_error("TODO 1: give the queues a capacity and start the worker");
        }
        m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(queue_capacity);
        m_from_worker = moodycamel::ReaderWriterQueue<ModelBlock>(queue_capacity);

        m_running.store(true, std::memory_order_release);
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
        // The audio thread. Take the host's samples in, hand whole model blocks
        // over to the worker, collect what came back, and give the host its
        // num_samples. Nothing here may allocate, lock or wait:
        //   - try_enqueue() and try_dequeue() fail rather than block — and if
        //     the queue is full, the worker is behind: deal with it here.
        //   - only take as much out of from_worker as m_output has room for.
        //   - when nothing has come back yet, the host still needs samples.
        // --------------------------------------------------------------------
    }

    // Everything the model produced, in order — what the check reads.
    const std::vector<float>& produced() const { return m_produced; }

    // Blocks the audio thread handed over that nobody picked up.
    size_t pending() const { return m_to_worker.size_approx(); }

private:
    // The worker thread: the engine lives here, and everything the engine does
    // — allocating, locking, growing arenas — is allowed on this side.
    void worker() {
        ModelBlock block;
        while (m_running.load(std::memory_order_acquire)) {
            // ---- TODO 3 ----------------------------------------------------
            // The worker thread. Take a block out of m_to_worker, run the
            // engine on it, and hand it back through m_from_worker. Append
            // every block to m_produced — that is what the check reads.
            // Nothing here is real-time: sleeping, waiting and allocating are
            // all allowed on this side.
            // ------------------------------------------------------------------
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // The loop ends while the host may still have pushed blocks, so TODO 3
        // also has to drain what is left in m_to_worker here.
    }

    LibTorchEngine& m_engine;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_to_worker{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_from_worker{0};
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::vector<float> m_produced;  // Written on the worker thread only
};

}  // namespace

int main() {
    std::unique_ptr<LibTorchEngine> engine;
    try {
        engine = std::make_unique<LibTorchEngine>(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    ThreadedProcessor processor(*engine);
    try {
        processor.prepare(k_host_block_size);
    } catch (const std::runtime_error& error) {
        std::printf("%s\n", error.what());
        return 1;
    }

    // The host now runs in real time: the worker has to keep up with a block of
    // audio every 10.7 ms, rather than with a loop going as fast as it can.
    g_allocations.store(0, std::memory_order_relaxed);

    run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host_block_size,
        [&processor](float* samples, size_t num_samples) {
            processor.process_block(samples, num_samples);
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
