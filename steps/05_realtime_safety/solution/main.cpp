// Step 5 — Real-time safety, demonstration
//
// Goal:  show that the engine is not real-time safe. Not a question of being
//        fast enough: it allocates, locks and calls into the system.
// Given: the ONNX Runtime engine and a RealtimeSanitizer (clang -fsanitize=realtime).
// Check: RTSan stops at the first violation inside the audio callback.
//
// There is no exercise here. Build it with -DWORKSHOP_RTSAN=ON and a clang that
// has RTSan, run it, and read what comes out.

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/helpers/onnx_engine.h"
#include "common/helpers/support.h"

// Disable RTSan while counting so it does not stop at the first violation or
// symbolize every stack trace. The allocation counter records the violations
// directly and keeps the run fast.
#if defined(__has_feature)
#if __has_feature(realtime_sanitizer)
#define WORKSHOP_HAS_RTSAN 1
extern "C" void __rtsan_disable();
extern "C" void __rtsan_enable();
#endif
#endif

// Count allocations independently of RTSan so this measurement works in both
// sanitized and unsanitized builds. The counter records only new/delete calls
// that occur while the audio callback is running.
namespace {
std::atomic<bool> g_in_callback{false};
std::atomic<long> g_allocations{0};
}  // namespace

void* operator new(size_t size) {
    if (g_in_callback.load(std::memory_order_relaxed)) {
        g_allocations.fetch_add(1, std::memory_order_relaxed);
    }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) { throw std::bad_alloc(); }
    return memory;
}

void operator delete(void* memory) noexcept {
    if (g_in_callback.load(std::memory_order_relaxed)) {
        g_allocations.fetch_add(1, std::memory_order_relaxed);
    }
    std::free(memory);
}

void operator delete(void* memory, size_t) noexcept {
    operator delete(memory);
}

// Model settings taken from the export metadata.
// @m_path: the ONNX model, state in and out
// @m_input_size: samples the model takes per forward pass
// @m_state_size: floats in the model's state
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
} k_model{};

// What an audio callback promises: it returns in bounded time, so it must not
// allocate, lock, or call the kernel. RTSan checks that promise and reports the
// first thing that breaks it.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

namespace {

// OnnxEngine with the state carried here, as in step 2. The state is allocated
// once, up front: anything the callback then reports comes from the engine.
class OnnxStream {
public:
    explicit OnnxStream(const char* model_path)
        : m_engine(model_path), m_state(k_model.m_state_size, 0.0f) {}

    void reset() { std::fill(m_state.begin(), m_state.end(), 0.0f); }

    void process(float* samples, size_t num_samples) {
        m_engine.process(samples, num_samples, m_state);
    }

private:
    OnnxEngine m_engine;
    std::vector<float> m_state;
};

// The audio thread, as far as this example goes: one block in, one block out,
// nothing else. Marking it is what turns RTSan on for everything it calls.
void audio_callback(OnnxStream& engine,
                    float* samples,
                    size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
    engine.process(samples, num_samples);
}

std::vector<float> one_block() {
    return {k_input_signal.data(), k_input_signal.data() + k_model.m_input_size};
}

// Runs several blocks through the callback and counts what the engine does on
// the way. The audio thread would have to survive every one of these.
// @engine: the engine under test
// @blocks: how many callbacks to run
void count_violations(OnnxStream& engine, int blocks) {
    std::vector<float> block = one_block();

    engine.reset();
    engine.process(block.data(), block.size());  // Warm up outside the callback

    std::printf("ONNX Runtime, %d blocks through the callback:\n", blocks);
#ifdef WORKSHOP_HAS_RTSAN
    __rtsan_disable();
#endif

    long total = 0;
    for (int i = 0; i < blocks; ++i) {
        g_allocations.store(0, std::memory_order_relaxed);
        g_in_callback.store(true, std::memory_order_relaxed);

        audio_callback(engine, block.data(), block.size());

        g_in_callback.store(false, std::memory_order_relaxed);
        const long count = g_allocations.load(std::memory_order_relaxed);
        total += count;
        std::printf("  block %2d   %6ld allocations and frees\n", i, count);
    }

#ifdef WORKSHOP_HAS_RTSAN
    __rtsan_enable();
#endif

    std::printf("  total      %6ld over %d blocks, %ld per block\n\n",
                total,
                blocks,
                total / blocks);
}

// The engine has to produce the reference output. Worth checking before
// claiming anything about it.
void verify(OnnxStream& engine) {
    std::vector<float> audio(k_input_signal.begin(), k_input_signal.end());

    engine.reset();
    for (size_t offset = 0; offset < audio.size(); offset += k_model.m_input_size) {
        engine.process(audio.data() + offset, k_model.m_input_size);
    }

    report_line("onnx", max_abs_diff(audio.data(), k_target_output_signal.data(), audio.size()));
}

}  // namespace

int main(int argc, char** argv) {
    const std::string which = argc > 1 ? argv[1] : "";

    try {
        OnnxStream engine(k_model.m_path);

        if (which == "count") {
            count_violations(engine, 10);
            return 0;
        }

        if (which == "verify") {
            verify(engine);
            return 0;
        }

        std::printf("Calling ONNX Runtime inference from a [[clang::nonblocking]] function.\n");
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
        std::printf(
            "Built with the attribute. With -fsanitize=realtime this stops at the\n"
            "first allocation, lock or syscall the engine makes.\n\n");
#else
        std::printf(
            "This compiler does not know [[clang::nonblocking]], so nothing is\n"
            "checked. Use a clang with RTSan; see the README.\n\n");
#endif

        std::vector<float> block = one_block();
        engine.process(block.data(), block.size());  // Warm up outside the callback
        audio_callback(engine, block.data(), block.size());
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    std::printf(
        "Returned without a report. Either RTSan is not enabled, or this\n"
        "engine really did nothing unsafe — check how it was built.\n");
    return 0;
}
