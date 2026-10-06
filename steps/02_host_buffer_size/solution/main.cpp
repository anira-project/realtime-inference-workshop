// Step 2 — Model input size vs. host buffer size, reference implementation
//
// Goal:  feed a model that only takes 2048 samples from a host that hands you
//        whatever it likes.
// Given: common/ring_buffer.h and common/host.h.
// Check: the stream the model produced is the same for every host block size.

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

// Host block sizes to test. The host's block size does not have to match the
// model's fixed 2048-sample input size; the ring buffers bridge that mismatch.
constexpr std::array<size_t, 6> k_host_block_sizes = {64, 128, 480, 512, 1024, 2048};

namespace {

// Like a plugin, allocate buffers after the host reports its block size in
// prepare(). The host then makes repeated process_block() calls. Ring buffers
// translate between the host's block size and the model's fixed block size,
// with one model block between an input and an output ring buffer.
class ProcessorExample {
public:
    explicit ProcessorExample(workshop::LibTorchEngine& engine, size_t model_input_size)
        : m_engine(engine), m_model_input_size(model_input_size) {}

    // Allocate and initialize all processing buffers before audio processing begins.
    // @max_block_size: the largest block process_block() can be given
    void prepare(size_t max_block_size) {
        // Room for a full host block on top of a full model block: the host can
        // write before the model has taken anything out.
        ringBuffersize = max_block_size + m_model_input_size;

        m_input = workshop::RingBuffer(ringBuffersize);
        m_output = workshop::RingBuffer(ringBuffersize);
        
        m_block.assign(m_model_input_size, 0.0f);
        m_produced.clear();
        m_engine.reset();
    }

    // Processes one block supplied by the host. The same buffer is used for input
    // and output, and its length is given by num_samples.
    void process_block(float* samples, size_t num_samples) {
        // Adds the host-provided samples to the input side.
        m_input.push(samples, num_samples);

        // Whole model blocks only — the rest waits for the next callback.
        while (m_input.available() >= m_model_input_size) {
            m_input.pop(m_block.data(), m_model_input_size);
            m_engine.process(m_block.data(), m_model_input_size);
            m_output.push(m_block.data(), m_model_input_size);
            m_produced.insert(m_produced.end(), m_block.begin(), m_block.end());
        }

        // Hand back what is ready. At the start nothing is, so the host gets
        // silence — that is latency, and it has its own step.
        if (m_output.available() >= num_samples) {
            m_output.pop(samples, num_samples);
        } else {
            std::fill_n(samples, num_samples, 0.0f);
        }
    }

    // Stores the model's output blocks in processing order for the final check.
    // It grows as process_block() runs; this is fine for the exercise, but
    // a real-time implementation would avoid expanding a vector during audio processing.
    const std::vector<float>& produced() const { return m_produced; }

private:
    workshop::LibTorchEngine& m_engine;
    size_t m_model_input_size;
    workshop::RingBuffer m_input{0};
    workshop::RingBuffer m_output{0};
    std::vector<float> m_block;
    std::vector<float> m_produced;
};

}  // namespace

int main() {
    std::unique_ptr<workshop::LibTorchEngine> engine;
    try {
        engine = std::make_unique<workshop::LibTorchEngine>(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    const std::array<float, workshop::k_signal_length>& input = workshop::k_input_signal;
    const std::array<float, workshop::k_signal_length>& target = workshop::k_target_output_signal;
    const auto model_input_size = static_cast<size_t>(k_model.m_input_size);

    ProcessorExample processor(*engine, model_input_size);
    bool all_ok = true;

    for (const size_t host_block_size : k_host_block_sizes) {
        const std::string label = "host block " + std::to_string(host_block_size);

        try {
            processor.prepare(host_block_size);
            workshop::run_host(input.data(),
                               input.size(),
                               host_block_size,
                               [&processor](float* samples, size_t num_samples) {
                                   processor.process_block(samples, num_samples);
                               });
        } catch (const std::exception& error) {
            std::printf("  %-24s %s\n",
                        label.c_str(),
                        workshop::error_summary(error.what()).c_str());
            all_ok = false;
            continue;
        }

        // The processed samples are compared with the reference output. Only complete
        // model-sized blocks can be checked, so the samples produced for the
        // complete portion of the host input are validated.
        const std::vector<float>& produced = processor.produced();
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

        all_ok &= workshop::report_line(
            label.c_str(),
            workshop::max_abs_diff(produced.data(), target.data(), produced.size()));
    }

    if (all_ok) {
        std::printf("\nOK: the model saw the same stream at every host block size.\n");
        return 0;
    }
    std::printf("\nFAILED.\n");
    return 1;
}
