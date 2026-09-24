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

// Runs the whole signal through the host at one block size, and returns
// everything the model produced, in order.
// @engine: the model, which only accepts model_input_size samples
// @signal: the input
// @host_block_size: what the host hands over per callback
// @model_input_size: what the model takes per forward pass
std::vector<float> run_at_block_size(workshop::LibTorchEngine& engine,
                                     const std::vector<float>& signal,
                                     size_t host_block_size,
                                     size_t model_input_size) {
    // Room for a full host block on top of a full model block: the host can
    // write before the model has taken anything out.
    workshop::RingBuffer input(host_block_size + model_input_size);
    workshop::RingBuffer output(host_block_size + model_input_size);

    std::vector<float> block(model_input_size);  // One model block, reused
    std::vector<float> produced;

    workshop::run_host(signal.data(),
                       signal.size(),
                       host_block_size,
                       [&](float* samples, size_t num_samples) {
                           input.push(samples, num_samples);

                           // Whole model blocks only — the rest waits for the next callback.
                           while (input.available() >= model_input_size) {
                               input.pop(block.data(), model_input_size);
                               engine.process(block.data(), model_input_size);
                               output.push(block.data(), model_input_size);
                               produced.insert(produced.end(), block.begin(), block.end());
                           }

                           // Hand back what is ready. At the start nothing is, so the host
                           // gets silence — that is latency, and it has its own step.
                           if (output.available() >= num_samples) {
                               output.pop(samples, num_samples);
                           } else {
                               std::fill_n(samples, num_samples, 0.0f);
                           }
                       });

    return produced;
}

}  // namespace

int main() {
    std::unique_ptr<workshop::LibTorchEngine> engine;
    try {
        engine = std::make_unique<workshop::LibTorchEngine>(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    const std::vector<float> input(workshop::k_input_signal.begin(),
                                   workshop::k_input_signal.end());
    const std::array<float, workshop::k_signal_length>& target = workshop::k_target_output_signal;
    const auto model_input_size = static_cast<size_t>(k_model.m_input_size);

    bool all_ok = true;
    for (const size_t host_block_size : k_host_block_sizes) {
        engine->reset();  // Every run starts from the same state

        const std::string label = "host block " + std::to_string(host_block_size);
        std::vector<float> produced;
        try {
            produced = run_at_block_size(*engine, input, host_block_size, model_input_size);
        } catch (const std::exception& error) {
            std::printf("  %-24s %s\n",
                        label.c_str(),
                        workshop::error_summary(error.what()).c_str());
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
