// Step 2 — Model input size vs. host buffer size
//
// Goal:   feed a model that only takes 2048 samples from a host that hands you
//         whatever it likes.
// Given:  common/ring_buffer.h and common/host.h — read both first.
// You do: fill in ProcessorExample, at the three TODO banners.
// Check:  the stream the model produced is the same for every host block size.

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

// What the export fixed, from its metadata. Sample rate and channel count are
// not used here — they are what the model assumes about the audio it is given.
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

// The shape a plugin has: the host says what it will do in prepare(), then
// calls process_block() over and over. Here, one model block in the middle and
// a ring buffer on each side.
class ProcessorExample {
public:
    explicit ProcessorExample(LibTorchEngine& engine, size_t model_input_size)
        : m_engine(engine), m_model_input_size(model_input_size) {}

    // Everything that allocates happens here, before the audio starts.
    // @max_block_size: the largest block process_block() will be given
    void prepare(size_t max_block_size) {
        // ---- TODO 1 --------------------------------------------------------
        // How much room does each ring buffer need? Think about the worst
        // moment: the host has just written a block and the model has not taken
        // anything out yet. Too little and push() throws.
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

    // One host block, in place: num_samples in, num_samples out.
    void process_block(float* samples, size_t num_samples) {
        // ---- TODO 2 --------------------------------------------------------
        // Take the host's samples in, and run the model whenever a whole block
        // has arrived. m_block is there to hold one model block. Append every
        // block the model returns to m_produced — that is what the check reads.
        // --------------------------------------------------------------------

        // ---- TODO 3 --------------------------------------------------------
        // Give the host its num_samples back, in place. At the start the model
        // has not produced anything yet — so what goes out?
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

        // The model saw whole blocks, so what it produced has to match the
        // reference for as far as it got — and it has to have got that far.
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
