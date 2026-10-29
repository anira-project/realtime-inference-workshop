// Step 2 — Model input size vs. host buffer size
//
// Goal:   feed a model that only takes 2048 samples from a host that hands you
//         whatever it likes.
// Given:  common/helpers/ring_buffer.h and common/helpers/host.h — read both first.
// Task:   fill in ProcessorExample, at the three TODO banners.
// Check:  is the stream the model produced the same for every host block size?

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/helpers/host.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/ring_buffer.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

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

// Block sizes a host might pick. Only 2048 happens to match the model. 441 is
// the awkward one: it is a whole number of milliseconds at 44.1 kHz, but neither
// a divisor of 2048 nor of the test signal, so that run stops a little early.
constexpr std::array<size_t, 6> k_host_block_sizes = {64, 128, 441, 512, 1024, 2048};

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
        // Choose a ring buffer size: a capacity that can absorb the mismatch
        // between the host block size and the model block size. Think about the
        // worst moment — the host has just delivered a block, the model has just
        // produced one, and the one before it has not been drained yet. This
        // happens as soon as the host block does not divide the model block.
        // Otherwise push() throws.
        // --------------------------------------------------------------------
        const size_t capacity = 0; // <- fill out

        if (capacity == 0) {
            throw std::runtime_error("TODO 1: give the ring buffers a capacity in prepare()");
        }

        m_input = RingBuffer(capacity);
        m_output = RingBuffer(capacity);

        // Given: room for exactly one model block. A block popped from m_input
        // lands here, the model processes it in place, and it is pushed on to
        // m_output from here.
        m_model_block.assign(m_model_input_size, 0.0f);

        m_produced.clear();
        m_engine.reset();
    }

    // Processes one block supplied by the host. The same buffer is used for input
    // and output, and its length is given by num_samples.
    void process_block(float* samples, size_t num_samples) {
        // ---- TODO 2 --------------------------------------------------------
        // From the host through the model:
        //   1. push the host's samples into m_input
        //   2. check if m_input has enough samples for a model block
        //   3. for each block: pop it into m_model_block, run the model (given),
        //      push the result into m_output
        //
        // Hint: available() says how many samples are waiting
        // --------------------------------------------------------------------
        // m_input.push(?, ?); // <- 1. fill out

        const size_t num_model_blocks = 0; // <- 2. fill out

        for (size_t b = 0; b < num_model_blocks; ++b) {
            // m_input.pop(?, ?); // <- 3. fill out
            m_engine.process(m_model_block.data(), m_model_input_size);
            // m_output.push(?, ?); // <- 3. fill out

            m_produced.insert(m_produced.end(), m_model_block.begin(), m_model_block.end());
        }

        // ---- TODO 3 --------------------------------------------------------
        // From m_output back to the host: pop num_samples straight into the
        // host's buffer, if there are enough samples waiting. At the start they are not,
        // and the host gets silence.
        // --------------------------------------------------------------------
        if (m_output.available() >= 0) { // <- fill out
            // m_output.pop(?, ?); // <- fill out
        } else {
            std::fill_n(samples, num_samples, 0.0f);
        }

        // After a finished callback, less than one model block waits for the
        // model, and at most one model block plus one host block for the host:
        // anything more means samples are piling up.
        if (m_input.available() >= m_model_input_size) {
            throw std::runtime_error("TODO 2: run the model on the samples that arrived");
        }
        if (m_output.available() > m_model_input_size + num_samples) {
            throw std::runtime_error("TODO 3: give the host its samples back");
        }
    }

    // Everything the model produced, in order — what the check reads. It grows
    // inside process_block(), which is fine offline and a problem later.
    const std::vector<float>& produced() const { return m_produced; }

private:
    LibTorchEngine& m_engine;
    size_t m_model_input_size;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    std::vector<float> m_model_block;
    std::vector<float> m_produced;
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

    ProcessorExample processor(engine, model_input_size);
    bool all_ok = true;

    for (const size_t host_block_size : k_host_block_sizes) {
        const std::string label = "host block " + std::to_string(host_block_size);

        std::vector<float> host_output;
        try {
            processor.prepare(host_block_size);
            host_output = run_host(input.data(),
                                   input.size(),
                                   host_block_size,
                                   [&processor](float* samples, size_t num_samples) {
                                       processor.process_block(samples, num_samples);
                                   });
        } catch (const std::exception& error) {
            // Stop at the first error: the other block sizes would only repeat it.
            std::printf("  %-24s %s\n", label.c_str(), error_summary(error.what()).c_str());
            std::printf("\nFAILED.\n");
            return 1;
        }

        // Nothing came out of the model at all.
        const std::vector<float>& produced = processor.produced();
        if (produced.empty()) {
            std::printf("  %-24s TODO 2: the model never ran   <-- FAILED\n", label.c_str());
            all_ok = false;
            continue;
        }

        // The host has to be given something back, even if it is silence.
        if (std::equal(host_output.begin(), host_output.end(), input.begin())) {
            std::printf("  %-24s TODO 3: the host got its own input back   <-- FAILED\n",
                        label.c_str());
            all_ok = false;
            continue;
        }

        // The model ran, but none of it reached the host.
        if (std::all_of(host_output.begin(), host_output.end(), [](float x) { return x == 0.0f; })) {
            std::printf("  %-24s TODO 2: nothing was pushed into m_output   <-- FAILED\n",
                        label.c_str());
            all_ok = false;
            continue;
        }

        // The processed samples are compared with the reference output. Only complete
        // model-sized blocks can be checked, so the samples produced for the
        // complete portion of the host input are validated.
        const size_t host_samples = input.size() / host_block_size * host_block_size;
        const size_t expected = host_samples / model_input_size * model_input_size;

        if (produced.size() != expected) {
            std::printf("  %-24s produced %zu samples, expected %zu   <-- FAILED\n",
                        label.c_str(),
                        produced.size(),
                        expected);
            all_ok = false;
            continue;
        }

        all_ok &= report_line(label.c_str(),
                              max_abs_diff(produced.data(), target.data(), produced.size()));
    }

    if (all_ok) {
        std::printf("\nOK: the model saw the same stream at every host block size.\n");
        return 0;
    }
    std::printf("\nFAILED.\n");
    return 1;
}
