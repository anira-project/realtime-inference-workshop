// Step 10 — the check: drives SplitProcessor the way the plugin's host does,
// prepare once, process every callback, on the fake host with the test signal.
// It runs offline, as a bounce: the host calls as fast as it can and every
// block waits for its result, so nothing is missed and the comparison is exact.
// First with every offset at 0, against step 10's reference
// (target_split_signal.h), shifted by latency(); then once more with offsets,
// which must change the output. Built twice: step10_exercise_check and
// step10_solution_check.

#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

#include "SplitProcessor.h"
#include "common/assets/target_split_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/anira_report.h"
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

using Offsets = std::array<float, SplitProcessor::k_num_latents>;

// The test signal through the processor, offline, with these offsets. `late`
// collects where the miss policy filled a block: offline it has to stay empty.
std::vector<float> run(SplitProcessor& processor, const Offsets& offsets, std::vector<size_t>& late) {
    size_t position = 0;
    return run_host(k_input_signal.data(), k_input_signal.size(), k_host.m_block_size,
                    [&](float* samples, size_t num_samples) {
                        if (processor.process(samples, num_samples, offsets, true) == ANIRA_MISSED) {
                            late.push_back(position);
                        }
                        position += num_samples;
                    });
}

}  // namespace

static int check() try {
    std::vector<float> output;
    size_t latency = 0;
    std::vector<size_t> late;
    {
        SplitProcessor processor(WORKSHOP_STEP_DIR);  // Every offset at 0
        processor.prepare(k_host.m_sample_rate, k_host.m_block_size);
        latency = processor.latency();
        print_report("encoder", processor.encoder_plan());
        print_report("decoder", processor.decoder_plan());
        std::printf("  %-28s %zu samples (%.1f ms at %.0f Hz)\n",
                    "reported latency",
                    latency,
                    1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                    k_host.m_sample_rate);

        output = run(processor, Offsets{}, late);
        std::printf("  latent frames                %zu, %zu of them priming\n",
                    processor.frames(),
                    processor.encoder_latency_frames());
    }
    if (!late.empty()) {
        fail("offline, %zu blocks were missed: that should not happen", late.size());
        return 2;
    }

    // The reference carries the model's own delay, which the decoder declared:
    // it lines up after the rest, as in steps 7 and 9.
    const int result = report_with_latency(output.data(), k_target_split_signal.data(), output.size(),
                                           latency - k_model_latency, late, k_host.m_block_size);
    if (result != 0) { return result; }

    // The knobs: offsets on every dimension must reach the decoder.
    SplitProcessor processor(WORKSHOP_STEP_DIR);
    const Offsets offsets = {1.6f, 2.8f, 2.3f, 0.7f, -2.0f, 2.7f, -2.9f, -1.2f};
    processor.prepare(k_host.m_sample_rate, k_host.m_block_size);
    std::vector<size_t> late_with_offset;
    const std::vector<float> with_offset = run(processor, offsets, late_with_offset);
    if (with_offset == output) {
        fail("TODO 2: the offsets do not change the output: add offsets[i] to each latent");
        return 2;
    }
    pass("the offsets change the output");
    return 0;
} catch (const std::exception& error) {
    // Every refusal, from the files to prepare, names what it refused
    fail("%s", error.what());
    return 2;
}

int main() {
    return step_result(10, check());
}
