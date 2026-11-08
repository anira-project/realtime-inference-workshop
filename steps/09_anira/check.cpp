// Step 9 — the check: drives AniraProcessor the way the plugin's host does,
// prepare once, process every callback, on the fake host with the test
// signal, and compares the output with the reference, shifted by latency().
// Built twice: step09_exercise_check and step09_solution_check.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

#include "AniraProcessor.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/anira_report.h"
#include "common/helpers/host.h"
#include "common/helpers/support.h"

// What the fake host does, in samples at its rate.
constexpr struct {
    size_t m_block_size = 512;
    double m_sample_rate = 44100.0;
} k_host{};

// The model's own delay, as declared in model.json. The reference output
// already carries it, and so does the latency anira reports.
constexpr size_t k_model_latency = 2224;

// Offline, the host's blocks change size: any size up to its largest may come,
// as the contract declares, so the check sends a few, not only 512.
constexpr std::array<size_t, 6> k_offline_blocks = {512, 100, 412, 1, 511, 256};

// The shift at which the output lines up with the reference, within tolerance;
// `num_samples` if it never does. The output is the reference, delayed.
static size_t matching_shift(const float* got, const float* want, size_t num_samples) {
    for (size_t shift = 0; shift < num_samples / 2; ++shift) {
        if (max_abs_diff(got + shift, want, num_samples - shift) <= k_tolerance) { return shift; }
    }
    return num_samples;
}

// The host offline: blocks of the given sizes in turn, each waiting for its
// result. Returns the output; `late` gets where a block was missed.
static std::vector<float> run_offline(AniraProcessor& processor,
                                      const std::vector<size_t>& block_sizes,
                                      std::vector<size_t>& late) {
    std::vector<float> output(k_input_signal.begin(), k_input_signal.end());
    late.clear();
    for (size_t offset = 0, call = 0; offset < output.size(); ++call) {
        const size_t size = std::min(block_sizes[call % block_sizes.size()], output.size() - offset);
        if (processor.process(output.data() + offset, size, true) != ANIRA_OK) { late.push_back(offset); }
        offset += size;
    }
    return output;
}

static int check() try {
    AniraProcessor processor(WORKSHOP_STEP_DIR);
    processor.prepare(k_host.m_sample_rate, k_host.m_block_size);
    const size_t latency = processor.latency();

    if (latency == 0) {
        fail("TODO 3: read the latency of output slot 0 into m_latency");
        return 2;
    }
    if (latency < k_model_latency) {
        fail("TODO 1: audio_out declares no latency: the model's own delay is %zu samples",
                     k_model_latency);
        return 2;
    }

    print_report("model", processor.plan());
    std::vector<size_t> late;
    std::printf("  %-28s %zu samples (%.1f ms at %.0f Hz)\n",
                "reported latency",
                latency,
                1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                k_host.m_sample_rate);

    // Offline, as a bounce: the host calls as fast as it can, and every block
    // waits for its result. Nothing may be missed, so the comparison is exact.
    // First with blocks of 512: a miss here is the call that does not wait.
    std::vector<float> output = run_offline(processor, {k_host.m_block_size}, late);
    if (std::equal(output.begin(), output.end(), k_input_signal.begin())) {
        fail("TODO 3: offline, the output is the input: process the block with process_wait");
        return 2;
    }
    if (!late.empty()) {
        fail("TODO 3: offline, %zu blocks were missed: wait for the result,\n"
             "process_wait with ANIRA_WAIT_FOREVER", late.size());
        return 2;
    }

    // Then with blocks of changing size, as the contract allows: a miss now is
    // a block size the contract leaves out.
    processor.prepare(k_host.m_sample_rate, k_host.m_block_size);
    output = run_offline(processor, {k_offline_blocks.begin(), k_offline_blocks.end()}, late);
    if (!late.empty()) {
        fail("TODO 3: offline, %zu blocks were missed once they changed size: the host may\n"
             "send any block from 1 up to its largest, so the contract has to say so",
             late.size());
        return 2;
    }

    // The reference already carries the model's own delay, so it lines up with
    // the output after the rest of the reported latency, as in step 7.
    const int result = report_with_latency(output.data(), k_target_output_signal.data(), output.size(),
                                           latency - k_model_latency, late, k_host.m_block_size);
    if (result != 0) {
        // A wrong "latency" on audio_out moves the reported latency, not the
        // output: the output still lines up, at another shift. Name it.
        const size_t shift = matching_shift(output.data(), k_target_output_signal.data(), output.size());
        if (shift != output.size()) {
            fail("TODO 1: audio_out declares a latency of %zu samples; the model's own delay is %zu",
                 latency - shift, k_model_latency);
        }
        return result;
    }

    // Live, at the host's pace: the call the audio thread makes. It has to
    // process the block; whether one comes back late does not matter here.
    const std::vector<float> live = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host.m_block_size,
        [&](float* samples, size_t num_samples) { processor.process(samples, num_samples); },
        k_host.m_sample_rate);
    // The block is processed in place: untouched, it goes out as it came in.
    if (std::equal(live.begin(), live.end(), k_input_signal.begin())) {
        fail("TODO 3: live, the output is the input: process the block in process()");
        return 2;
    }
    pass("live, at the host's pace, the blocks are processed too");
    return 0;
} catch (const std::exception& error) {
    // Every refusal of anira's, from the files to prepare, names what it refused
    fail("%s", error.what());
    return 2;
}

int main() {
    return step_result(9, check());
}
