// Step 10 — the check: drives SplitProcessor the way the plugin's host does,
// prepare once, process every callback, on the fake host with the test
// signal. With zero offsets the output is compared with step 10's reference
// (target_split_signal.h), shifted by latency(); then once more with an offset
// on z0, which must change it. Built twice: step10_exercise_check and
// step10_solution_check.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

#include "SplitProcessor.h"
#include "common/assets/target_split_signal.h"
#include "common/assets/test_signal.h"
#include "common/helpers/host.h"
#include "common/helpers/support.h"

// What the fake host does, in samples at its rate.
constexpr struct {
    size_t m_block_size = 512;
    double m_sample_rate = 44100.0;
} k_host{};

// The model's own delay, as declared on the decoder's output. The reference
// already carries it, and so does the latency anira reports.
constexpr size_t k_model_latency = 2224;

namespace {

// The test signal through the processor on the fake host. `late` collects
// where the miss policy filled a block.
std::vector<float> run(SplitProcessor& processor, std::vector<size_t>& late) {
    size_t position = 0;
    return run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host.m_block_size,
        [&](float* samples, size_t num_samples) {
            if (processor.process(samples, num_samples)) { late.push_back(position); }
            position += num_samples;
        },
        k_host.m_sample_rate);
}

}  // namespace

int main() try {
    std::vector<float> output;
    size_t latency = 0;
    std::vector<size_t> late;
    {
        SplitProcessor processor(WORKSHOP_STEP_DIR);  // Every offset at 0
        processor.prepare(k_host.m_sample_rate, k_host.m_block_size);
        latency = processor.latency();

        if (latency == 0) {
            std::fprintf(stderr, "TODO 1: the latency the host is told: the encoder's, in samples, plus the decoder's.\n");
            return 2;
        }

        std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                    latency,
                    1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                    k_host.m_sample_rate);

        output = run(processor, late);

        if (processor.frames() == 0) {
            std::fprintf(stderr, "TODO 2: no latent frame came out of the encoder.\n");
            return 2;
        }
        // The block is the decoder's output: unpopped, it goes out as it came in.
        if (std::equal(output.begin(), output.end(), k_input_signal.begin())) {
            std::fprintf(stderr, "TODO 4: the output is the input: pop the decoder's audio into the block.\n");
            return 2;
        }
        std::printf("  latent frames                %zu, %zu of them priming\n",
                    processor.frames(),
                    processor.encoder_latency_frames());
    }

    // The reference carries the model's own delay, which the decoder declared:
    // it lines up after the rest, as in steps 7 and 9.
    const int result = report_with_latency(output.data(), k_target_split_signal.data(), output.size(),
                                           latency - k_model_latency, late, k_host.m_block_size);
    if (result != 0) { return result; }

    // The knob: an offset on z0 must reach the decoder.
    SplitProcessor processor(WORKSHOP_STEP_DIR);
    processor.set_offset(0, 1.0f);
    processor.prepare(k_host.m_sample_rate, k_host.m_block_size);
    std::vector<size_t> late_with_offset;
    const std::vector<float> with_offset = run(processor, late_with_offset);
    if (with_offset == output) {
        std::fprintf(stderr, "TODO 3: an offset on z0 does not change the output: add it to the latents.\n");
        return 2;
    }
    std::printf("OK: an offset of 1 on z0 changes the output.\n");
    return 0;
} catch (const std::exception& error) {
    // Every refusal of anira's, from the files to prepare, names what it refused
    std::fprintf(stderr, "%s\n", error.what());
    return 2;
}
