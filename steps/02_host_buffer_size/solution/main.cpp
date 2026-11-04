// Step 2 — Model input size vs. host buffer size, reference implementation
//
// Goal:   feed a model that only takes 2048 samples from a host that hands you
//         512 at a time.
// Given:  common/helpers/ring_buffer.h and common/helpers/host.h — read both first.
// Task:   fill in ProcessorExample, at the three TODO banners.
// Check:  does the host get the model's stream back, sample for sample?

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/helpers/host.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/ring_buffer.h"
#include "common/helpers/support.h"

// Model settings taken from the export metadata. The sample rate and channel
// count are not used by this exercise, but describe the audio format expected
// by the model.
// @m_path: absolute path to the exported model, set via CMake
// @m_input_size: samples the model takes per forward pass
// @m_sample_rate: the sample rate the model was trained for
// @m_channels: channels in and out
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 44100.0;
    int m_channels = 1;
} k_model{};

// What the host hands over per callback. Not the model's 2048, but it divides
// it: four callbacks make one model block.
constexpr size_t k_host_block_size = 512;

namespace {

// Like a plugin, allocate buffers after the host reports its block size in
// prepare(). The host then makes repeated process_block() calls. Two ring
// buffers translate between the host's block size and the model's fixed one:
//
//   host block -> m_input -> m_model_block -> model -> m_output -> host block
class ProcessorExample {
public:
    explicit ProcessorExample(LibTorchEngine& engine, size_t model_input_size)
        : m_engine(engine), m_model_input_size(model_input_size) {}

    // Allocate and initialize all processing buffers before audio processing begins.
    // @max_block_size: the largest block process_block() can be given
    void prepare([[maybe_unused]] size_t max_block_size) {
        // ---- TODO 1 --------------------------------------------------------
        // Choose a ring buffer size: a capacity that can hold everything that
        // is in flight at the worst moment. Think about the input side just
        // before a model block is taken out, and the output side just after
        // one was put in. Too small, and push() throws.
        // --------------------------------------------------------------------
        const size_t capacity = m_model_input_size;  // <- solution

        if (capacity == 0) {
            throw std::runtime_error("TODO 1: give the ring buffers a capacity in prepare()");
        }

        m_input = RingBuffer(capacity);
        m_output = RingBuffer(capacity);

        // Given: room for exactly one model block. A block popped from m_input
        // lands here, the model processes it in place, and it is pushed on to
        // m_output from here.
        m_model_block.assign(m_model_input_size, 0.0f);

        m_engine.reset();
    }

    // Processes one block supplied by the host. The same buffer is used for input
    // and output, and its length is given by num_samples.
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

        const bool has_block = m_input.available() >= m_model_input_size;  // <- solution

        if (has_block) {
            m_input.pop(m_model_block.data(), m_model_input_size);  // <- solution
            m_engine.process(m_model_block.data(), m_model_input_size);
            m_output.push(m_model_block.data(), m_model_input_size);  // <- solution
        }

        // ---- TODO 3 --------------------------------------------------------
        // From m_output back to the host: pop num_samples straight into the
        // host's buffer, if there are enough samples waiting. At the start they are not,
        // and the host gets silence.
        // --------------------------------------------------------------------
        if (m_output.available() >= num_samples) {  // <- solution
            m_output.pop(samples, num_samples);     // <- solution
        } else {
            std::fill_n(samples, num_samples, 0.0f);
        }

        // After a finished callback, less than one model block waits for the
        // model, and at most one model block for the host: anything more
        // means samples are piling up.
        if (m_input.available() >= m_model_input_size) {
            throw std::runtime_error("TODO 2: run the model on the samples that arrived");
        }
        if (m_output.available() > m_model_input_size) {
            throw std::runtime_error("TODO 3: give the host its samples back");
        }
    }

private:
    LibTorchEngine& m_engine;
    size_t m_model_input_size;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    std::vector<float> m_model_block;
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

    const std::array<float, k_signal_length>& input = k_input_signal;
    const std::array<float, k_signal_length>& target = k_target_output_signal;
    const auto model_input_size = static_cast<size_t>(k_model.m_input_size);
    const std::string label = "host block " + std::to_string(k_host_block_size);

    ProcessorExample processor(engine, model_input_size);

    std::vector<float> host_output;
    try {
        processor.prepare(k_host_block_size);
        host_output = run_host(input.data(),
                               input.size(),
                               k_host_block_size,
                               [&processor](float* samples, size_t num_samples) {
                                   processor.process_block(samples, num_samples);
                               });
    } catch (const std::exception& error) {
        std::printf("  %-24s %s\n", label.c_str(), error_summary(error.what()).c_str());
        std::printf("\nFAILED.\n");
        return 1;
    }

    // The host has to be given something back, even if it is silence.
    if (std::equal(host_output.begin(), host_output.end(), input.begin())) {
        std::printf("  %-24s TODO 3: the host got its own input back   <-- FAILED\n",
                    label.c_str());
        std::printf("\nFAILED.\n");
        return 1;
    }

    // Nothing reached the host at all.
    if (std::all_of(host_output.begin(), host_output.end(), [](float x) { return x == 0.0f; })) {
        std::printf("  %-24s TODO 2: nothing came back from the model   <-- FAILED\n",
                    label.c_str());
        std::printf("\nFAILED.\n");
        return 1;
    }

    // What the host got back: silence until the first model block was ready,
    // then the model's stream, sample for sample. How long that silence is
    // depends on your ring buffers, so every whole number of callbacks up to
    // two model blocks is tried, and the closest one counts.
    float closest = std::numeric_limits<float>::infinity();
    for (size_t delay = 0; delay <= 2 * model_input_size; delay += k_host_block_size) {
        closest = std::min(
            closest,
            diff_with_latency(host_output.data(), target.data(), host_output.size(), delay));
    }
    if (closest == std::numeric_limits<float>::infinity()) {
        std::printf(
            "  %-24s TODO 3: the host did not get silence first, then the model's stream   <-- "
            "FAILED\n",
            label.c_str());
        std::printf("\nFAILED.\n");
        return 1;
    }

    if (!report_line(label.c_str(), closest)) {
        std::printf("\nFAILED.\n");
        return 1;
    }
    std::printf("\nOK: the host got the model's stream back, after a stretch of silence.\n");
    return 0;
}
