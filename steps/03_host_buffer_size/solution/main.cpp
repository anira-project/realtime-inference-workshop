// Step 3 — Model input size vs. host buffer size, reference implementation
//
// Goal:   feed a model that only takes 2048 samples from a host that hands you
//         512 at a time.
// Given:  common/helpers/ring_buffer.h — read it first.
// Task:   fill in ProcessorExample, at the three TODO banners.
// Check:  does the host get the model's stream back, sample for sample?

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/step03.h"
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

// Shaped like a plugin: prepare() allocates before the audio starts, then the
// host calls process_block() once per callback. Two ring buffers translate
// between the host's block size and the model's:
//
//   host block -> m_input -> m_model_block -> model -> m_output -> host block
class ProcessorExample {
public:
    explicit ProcessorExample(OnnxEngine& engine) : m_engine(engine) {}

    void prepare(size_t max_block_size) {
        // ---- TODO 1 --------------------------------------------------------
        // Choose a ring buffer size: a capacity that can hold everything that
        // is in flight at the worst moment. Think about the input side just
        // before a model block is taken out, and the output side just after
        // one was put in. Too small, and push() throws.
        // --------------------------------------------------------------------
        const size_t capacity = k_model.m_input_size;  // <- solution

        m_input = RingBuffer(capacity);
        m_output = RingBuffer(capacity);
        m_model_block.assign(k_model.m_input_size, 0.0f);
        m_state.assign(k_model.m_state_size, 0.0f);
    }

    // One callback: the host's samples come in and go back out in place.
    void process_block(float* samples, size_t num_samples) {
        // ---- TODO 2 --------------------------------------------------------
        // From the host through the model:
        //   1. push the host's samples into m_input
        //   2. check if m_input has enough samples for a model block
        //   3. if so: pop it into m_model_block, run the model (given), push
        //      the result into m_output
        //
        // Hint: available() says how many samples are waiting
        // --------------------------------------------------------------------
        m_input.push(samples, num_samples);  // <- solution

        const bool has_block = m_input.available() >= k_model.m_input_size;  // <- solution

        if (has_block) {
            m_input.pop(m_model_block.data(), k_model.m_input_size);  // <- solution
            m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
            m_output.push(m_model_block.data(), k_model.m_input_size);  // <- solution
        }

        // ---- TODO 3 --------------------------------------------------------
        // From m_output back to the host: pop num_samples straight into the
        // host's buffer, if there are enough samples waiting. At the start
        // they are not, and the host gets silence.
        // --------------------------------------------------------------------
        if (m_output.available() >= num_samples) {  // <- solution
            m_output.pop(samples, num_samples);     // <- solution
        } else {
            std::fill_n(samples, num_samples, 0.0f);
        }

        check_todos(m_input, m_output, k_model.m_input_size);  // Given: names a missing TODO
    }

private:
    OnnxEngine& m_engine;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    std::vector<float> m_model_block;
    std::vector<float> m_state;
};

int main() {
    try {
        OnnxEngine engine(k_model.m_path);

        ProcessorExample processor(engine);
        processor.prepare(k_host_block_size);

        // The host: the signal in one buffer, handed over 512 samples at a
        // time and processed in place.
        std::vector<float> output(k_input_signal.begin(), k_input_signal.end());
        for (size_t start = 0; start < output.size(); start += k_host_block_size) {
            processor.process_block(output.data() + start, k_host_block_size);
        }

        // Silence first, then the model's stream.
        const size_t latency = k_model.m_input_size - k_host_block_size;
        return report_with_latency(output.data(),
                                   k_target_output_signal.data(),
                                   output.size(),
                                   latency);
    } catch (const std::exception& error) {
        std::printf("FAILED: %s\n", error.what());
        return 1;
    }
}
