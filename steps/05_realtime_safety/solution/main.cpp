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
constexpr struct {
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

// Step 3's processor, unchanged except for two things: the callback makes the
// promise, and prepare() warms the engine up, so what RTSan finds is not a
// first-call effect.
class ProcessorExample {
public:
    explicit ProcessorExample(OnnxEngine& engine) : m_engine(engine) {}

    void prepare(size_t max_block_size) {
        const size_t capacity = k_model.m_input_size;

        m_input = RingBuffer(capacity);
        m_output = RingBuffer(capacity);
        m_model_block.assign(k_model.m_input_size, 0.0f);
        m_state.assign(k_model.m_state_size, 0.0f);

        // The warm-up, before the audio starts. The state starts at zero again.
        m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
        std::fill(m_state.begin(), m_state.end(), 0.0f);
    }

    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
        m_input.push(samples, num_samples);

        const bool has_block = m_input.available() >= k_model.m_input_size;

        if (has_block) {
            m_input.pop(m_model_block.data(), k_model.m_input_size);
            m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
            m_output.push(m_model_block.data(), k_model.m_input_size);
        }

        if (m_output.available() >= num_samples) {
            m_output.pop(samples, num_samples);
        } else {
            std::fill_n(samples, num_samples, 0.0f);
        }
    }

private:
    OnnxEngine& m_engine;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    std::vector<float> m_model_block;
    std::vector<float> m_state;
};

int main() {
#ifndef WORKSHOP_HAS_RTSAN
    std::printf(
        "WARNING: built without RealtimeSanitizer, so nothing is checked.\n"
        "It needs clang >= 20 from LLVM (macOS: Homebrew LLVM, not Apple Clang; or Linux)\n"
        "and -DWORKSHOP_RTSAN=ON. See the README.\n\n");
#endif

    OnnxEngine engine(k_model.m_path);

    ProcessorExample processor(engine);
    processor.prepare(k_host_block_size);

    // The host, as in step 3. With RTSan, the run stops in the fourth callback,
    // the first one that runs the model.
    std::vector<float> output(k_input_signal.begin(), k_input_signal.end());
    for (size_t start = 0; start < output.size(); start += k_host_block_size) {
        processor.process_block(output.data() + start, k_host_block_size);
    }

    // Without RTSan the run gets here: the engine is still correct, just not safe.
    const size_t latency = k_model.m_input_size - k_host_block_size;
    return report_with_latency(output.data(),
                               k_target_output_signal.data(),
                               output.size(),
                               latency);
}
