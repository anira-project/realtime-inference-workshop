// Step 2 — Model input size vs. host buffer size
//
// Goal:   feed a model that only takes 2048 samples from a host that hands you
//         whatever it likes.
// Given:  common/ring_buffer.h and common/host.h — read both first.
// You do: fill in BlockAdapter, at the three TODO banners.
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

// Block sizes a host might pick. 2048 is the one case where it happens to match
// the model; the rest are ordinary, and one of them does not even divide it.
constexpr std::array<size_t, 6> k_host_block_sizes = {64, 128, 480, 512, 1024, 2048};

namespace {

// Sits between the two block sizes: the host writes into one ring buffer, the
// model reads whole blocks out of it, and what it produces goes into the other.
class BlockAdapter {
public:
    // @engine: the model, which only accepts model_input_size samples
    // @host_block_size: the largest block the host will hand over
    // @model_input_size: what the model takes per forward pass
    BlockAdapter(workshop::LibTorchEngine& engine, size_t host_block_size, size_t model_input_size)
        : m_engine(engine)
        , m_model_input_size(model_input_size)
        // ---- TODO 1 --------------------------------------------------------
        // How much room does each ring buffer need? Too little and push()
        // throws, which the report below shows you. Both sizes are in scope.
        // --------------------------------------------------------------------
        , m_input(0)
        , m_output(0)
        , m_block(model_input_size) {}

    // The host callback: num_samples in, num_samples out, in place.
    void process(float* samples, size_t num_samples) {
        // ---- TODO 2 ------------------------------------------------------------
        // Take the host's samples in, and run the model whenever a whole block
        // has arrived. m_block is there to hold one model block.
        // Remember to record what the model produced: m_produced is what the
        // check reads.
        // ------------------------------------------------------------------------

        // ---- TODO 3 ------------------------------------------------------------
        // Hand num_samples back to the host, in place. At the start the model
        // has not produced anything yet — so what do you give it?
        // ------------------------------------------------------------------------
    }

    // Everything the model produced, in order — the stream to check.
    const std::vector<float>& produced() const { return m_produced; }

private:
    workshop::LibTorchEngine& m_engine;
    size_t m_model_input_size;
    workshop::RingBuffer m_input;
    workshop::RingBuffer m_output;
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

    constexpr size_t k_test_signal_length = workshop::k_signal_length;
    const std::array<float, k_test_signal_length>& input = workshop::k_input_signal;
    const std::array<float, k_test_signal_length>& target = workshop::k_target_output_signal;

    const auto model_input_size = static_cast<size_t>(k_model.m_input_size);
    bool all_ok = true;

    for (const size_t host_block_size : k_host_block_sizes) {
        engine->reset();  // Every run starts from the same state
        BlockAdapter adapter(*engine, host_block_size, model_input_size);

        try {
            workshop::run_host(input.data(),
                               input.size(),
                               host_block_size,
                               [&adapter](float* samples, size_t num_samples) {
                                   adapter.process(samples, num_samples);
                               });
        } catch (const std::exception& error) {
            std::printf("  host block %-13zu %s\n",
                        host_block_size,
                        workshop::error_summary(error.what()).c_str());
            all_ok = false;
            continue;
        }

        // The model saw whole blocks, so what it produced has to match the
        // reference for as far as it got — and it has to have got that far.
        const std::string label = "host block " + std::to_string(host_block_size);
        const std::vector<float>& produced = adapter.produced();
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
