// Step 5 — Real-time safety, demonstration
//
// Goal:  show that the engine is not real-time safe. Not a question of being
//        fast enough: it allocates, locks and calls into the system.
// Given: step 3's processor, with its callback marked [[clang::nonblocking]]
//        (common/helpers/audio_callback.h), and RealtimeSanitizer
//        (clang -fsanitize=realtime).
// Check: RTSan stops at the first violation inside the callback.
//
// There is no exercise here. Build it with -DWORKSHOP_RTSAN=ON and a clang that
// has RTSan, run it, and read what comes out.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/helpers/audio_callback.h"
#include "common/helpers/onnx_engine.h"
#include "common/helpers/ring_buffer.h"
#include "common/helpers/support.h"

// Model settings taken from the export metadata, as in step 1.
constexpr struct ModelConfig {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
} k_model{};

// What the host hands over per callback: four callbacks make one model block.
constexpr size_t k_host_block_size = 512;

// Whether this build has RealtimeSanitizer (clang -fsanitize=realtime).
#if defined(__has_feature)
#if __has_feature(realtime_sanitizer)
#define WORKSHOP_HAS_RTSAN
#endif
#endif

// Step 3's processor, unchanged except for one thing: the callback makes the
// promise.
class ProcessorExample {
public:
    // Keeps the model and makes the engine, which loads nothing yet:
    // prepare() loads the model.
    explicit ProcessorExample(const ModelConfig& model) : m_model(model) {}

    void prepare(size_t host_block_size) {
        // The model, loaded the first time: it takes a while and allocates.
        if (!m_engine.loaded()) { m_engine.load(m_model.m_path); }

        const size_t prefill = m_model.m_input_size - host_block_size;

        m_input = RingBuffer(m_model.m_input_size);
        m_output = RingBuffer(prefill + m_model.m_input_size);
        for (size_t i = 0; i < prefill; ++i) {
            const bool pushed = m_output.try_push(0.0f);
            assert(pushed);  // never fails: m_output has room for the prefill
        }
        m_model_block.assign(m_model.m_input_size, 0.0f);
        m_state.assign(m_model.m_state_size, 0.0f);
    }

    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
        for (size_t i = 0; i < num_samples; ++i) {
            const bool pushed = m_input.try_push(samples[i]);
            assert(pushed);  // never fails: the model takes a block out before m_input is full
        }

        if (m_input.available() >= m_model.m_input_size) {
            for (size_t i = 0; i < m_model.m_input_size; ++i) {
                const bool popped = m_input.try_pop(m_model_block[i]);
                assert(popped);  // never fails: a whole block is there
            }
            m_engine.process(m_model_block.data(), m_model.m_input_size,
                             m_state.data(), m_model.m_state_size);
            for (size_t i = 0; i < m_model.m_input_size; ++i) {
                const bool pushed = m_output.try_push(m_model_block[i]);
                assert(pushed);  // never fails: m_output has room for a whole block
            }
        }

        for (size_t i = 0; i < num_samples; ++i) {
            const bool popped = m_output.try_pop(samples[i]);
            assert(popped);  // never fails: the prefill covers the wait for the first block
        }
    }

private:
    OnnxEngine m_engine;        // Made with the processor, loaded in prepare()
    const ModelConfig m_model;  // Given at construction, never changes
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    std::vector<float> m_model_block;
    std::vector<float> m_state;
};

int main() {
    const char* unchecked = nullptr;  // What the result adds to SOLVED
#ifndef WORKSHOP_HAS_RTSAN
    unchecked = "but real-time safety is not checked: no RealtimeSanitizer";
    note("built without RealtimeSanitizer, so real-time safety is not checked. It needs\n"
         "clang >= 20 from LLVM (macOS: Homebrew LLVM, not Apple Clang; or Linux) and\n"
         "-DWORKSHOP_RTSAN=ON. See the README.");
#endif

    ProcessorExample processor(k_model);  // Keeps the model, loads nothing yet
    processor.prepare(k_host_block_size);  // Loads the model

    // The host, as in step 3. With RTSan, the run stops in the fourth callback,
    // the first one that runs the model.
    std::vector<float> buffer(k_input_signal.begin(), k_input_signal.end());
    for (size_t start = 0; start < buffer.size(); start += k_host_block_size) {
        processor.process_block(buffer.data() + start, k_host_block_size);
    }

    // Without RTSan the run gets here: the engine is still correct, just not safe.
    const size_t latency = k_model.m_input_size - k_host_block_size;
    return step_result(5, report_with_latency(buffer.data(),
                                              k_target_output_signal.data(),
                                              buffer.size(),
                                              latency),
                       unchecked);
}
