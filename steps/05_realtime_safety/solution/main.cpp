// Step 5 — Real-time safety, demonstration
//
// Goal:  show that these engines are not real-time safe. Not a question of
//        being fast enough: they allocate, lock and call into the system.
// Given: both engines, and RealtimeSanitizer (clang -fsanitize=realtime).
// Check: RTSan stops at the first violation inside the audio callback.
//
// There is no exercise here. Build it with -DWORKSHOP_RTSAN=ON and a clang that
// has RTSan, run it, and read what comes out.

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/libtorch_engine.h"
#include "common/onnx_engine.h"
#include "common/support.h"
#include "common/target_signal.h"
#include "common/test_signal.h"

// Counting mode turns the sanitizer off while it runs: it counts for itself,
// and symbolizing a few hundred thousand stack traces would take minutes.
#if defined(__has_feature)
#if __has_feature(realtime_sanitizer)
#define WORKSHOP_HAS_RTSAN 1
extern "C" void __rtsan_disable();
extern "C" void __rtsan_enable();
#endif
#endif

// Counting the allocations ourselves, so this works in any build — RTSan or
// not. Every new and delete in the process goes through here; only the ones
// inside the callback are counted.
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
// @m_torchscript_path: the LibTorch model, state inside the model
// @m_onnx_path: the ONNX model, state in and out
// @m_input_size: samples the model takes per forward pass
constexpr struct {
    const char* m_torchscript_path = WORKSHOP_MODEL_PATH;
    const char* m_onnx_path = WORKSHOP_ONNX_MODEL_PATH;
    int m_input_size = 2048;
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

// The audio thread, as far as this example goes: one block in, one block out,
// nothing else. Marking it is what turns RTSan on for everything it calls.
template <typename Engine>
void audio_callback(Engine& engine, float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
    engine.process(samples, num_samples);
}

std::vector<float> one_block() {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    return {k_input_signal.begin(), k_input_signal.begin() + block_size};
}

// Runs several blocks through the callback and counts what the engine does on
// the way. The audio thread would have to survive every one of these.
// @engine: the engine under test
// @name: what to print
// @blocks: how many callbacks to run
template <typename Engine>
void count_violations(Engine& engine, const char* name, int blocks) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> block(k_input_signal.begin(), k_input_signal.begin() + block_size);

    engine.reset();
    engine.process(block.data(), block.size());  // Warm up outside the callback

    std::printf("%s, %d blocks through the callback:\n", name, blocks);
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

// Both engines run the same model, so both have to produce the reference
// output. Worth checking before claiming anything about either of them.
template <typename Engine>
void verify(Engine& engine, const char* name) {
    const auto block_size = static_cast<size_t>(k_model.m_input_size);
    std::vector<float> audio(k_input_signal.begin(), k_input_signal.end());

    engine.reset();
    for (size_t offset = 0; offset + block_size <= audio.size(); offset += block_size) {
        engine.process(audio.data() + offset, block_size);
    }

    report_line(name, max_abs_diff(audio.data(), k_target_output_signal.data(), audio.size()));
}

}  // namespace

int main(int argc, char** argv) {
    const std::string which = argc > 1 ? argv[1] : "libtorch";

    try {
        if (which == "count") {
            const std::string engine_name = argc > 2 ? argv[2] : "libtorch";
            constexpr int k_blocks = 10;

            if (engine_name == "onnx") {
                OnnxEngine engine(k_model.m_onnx_path);
                count_violations(engine, "ONNX Runtime", k_blocks);
            } else {
                LibTorchEngine engine(k_model.m_torchscript_path);
                count_violations(engine, "LibTorch", k_blocks);
            }
            return 0;
        }

        if (which == "verify") {
            LibTorchEngine libtorch(k_model.m_torchscript_path);
            OnnxEngine onnx(k_model.m_onnx_path);
            verify(libtorch, "libtorch");
            verify(onnx, "onnx");
            return 0;
        }

        std::printf("Calling %s inference from a [[clang::nonblocking]] function.\n",
                    which.c_str());
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

        if (which == "onnx") {
            OnnxEngine engine(k_model.m_onnx_path);
            engine.process(block.data(), block.size());  // Warm up outside the callback
            audio_callback(engine, block.data(), block.size());
        } else {
            LibTorchEngine engine(k_model.m_torchscript_path);
            engine.process(block.data(), block.size());  // Warm up outside the callback
            audio_callback(engine, block.data(), block.size());
        }
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    std::printf(
        "Returned without a report. Either RTSan is not enabled, or this\n"
        "engine really did nothing unsafe — check how it was built.\n");
    return 0;
}
