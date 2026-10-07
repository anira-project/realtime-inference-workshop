// Step 2 — Model input size vs. host buffer size
//
// Goal:   feed a model that only takes 2048 samples from a host that hands you
//         whatever it likes.
// Given:  common/ring_buffer.h and common/host.h — read both first.
// Task:   fill in ProcessorExample, at the three TODO banners.
// Check:  is the stream the model produced the same for every host block size?

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/host.h"
#include "common/libtorch_engine.h"
#include "common/ring_buffer.h"
#include "common/support.h"
#include "common/target_signal.h"
#include "common/test_signal.h"

// Model settings taken from the export metadata. The sample rate and channel
// count are not used by this exercise, but describe the audio format expected
// by the model.
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    int m_input_size = 2048;
    double m_sample_rate = 48000.0;
    int m_channels = 1;
} k_model{};

// Block sizes a host might pick. Only 2048 happens to match the model. 480 is
// the awkward one: it is a whole number of milliseconds at 48 kHz, but neither
// a divisor of 2048 nor of the test signal, so that run stops a little early.
constexpr std::array<size_t, 6> k_host_block_sizes = {64, 128, 480, 512, 1024, 2048};

namespace {

// Like a plugin, allocate buffers after the host reports its block size in
// prepare(). The host then makes repeated process_block() calls. Ring buffers
// translate between the host's block size and the model's fixed block size,
// with one model block between an input and an output ring buffer.
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
        const size_t capacity = 0;

        if (capacity == 0) {
            throw std::runtime_error("TODO 1: give the ring buffers a capacity in prepare()");
        }
        m_input = RingBuffer(capacity);
        m_output = RingBuffer(capacity);

        m_block.assign(m_model_input_size, 0.0f);
        m_produced.clear();
        m_engine.reset();
    }

    // Processes one block supplied by the host. The same buffer is used for input
    // and output, and its length is given by num_samples.
    void process_block(float* samples, size_t num_samples) {
        // ---- TODO 2 --------------------------------------------------------
        // Add the host-provided samples to the input side, then process any complete
        // model-sized blocks that are available. Move each block through the model,
        // make its output available to the host, and Append every
        // block the model returns to m_produced, so the check can read it.
        // --------------------------------------------------------------------

        // ---- TODO 3 --------------------------------------------------------
        // Write exactly num_samples output samples into the host-provided buffer. Use any
        // processed samples that are ready; before the first model block is complete,
        // decide what value should fill the unavailable portion.
        // --------------------------------------------------------------------

        // After a finished callback, less than one model block is waiting on
        // either side: anything else means samples are piling up.
        if (m_input.available() >= m_model_input_size) {
            throw std::runtime_error("TODO 2: run the model on the samples that arrived");
        }
        if (m_output.available() >= m_model_input_size) {
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
    std::vector<float> m_block;
    std::vector<float> m_produced;
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

    const std::array<float, k_signal_length>& input = k_input_signal;
    const std::array<float, k_signal_length>& target = k_target_output_signal;
    const auto model_input_size = static_cast<size_t>(k_model.m_input_size);

    ProcessorExample processor(*engine, model_input_size);
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
            std::printf("  %-24s %s\n", label.c_str(), error_summary(error.what()).c_str());
            all_ok = false;
            continue;
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
