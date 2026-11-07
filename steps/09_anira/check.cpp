// Step 9 — the check: drives AniraProcessor the way the plugin's host does,
// prepare once, process every callback, on the fake host with the test
// signal, and compares the output with the reference, shifted by latency().
// Built twice: step09_exercise_check and step09_solution_check.

#include <algorithm>
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

int main() try {
    AniraProcessor processor(WORKSHOP_STEP_DIR);
    processor.prepare(k_host.m_sample_rate, k_host.m_block_size);
    const size_t latency = processor.latency();

    if (latency == 0) {
        std::fprintf(stderr, "TODO 3: read the latency of output slot 0 into m_latency.\n");
        return 2;
    }
    if (latency < k_model_latency) {
        std::fprintf(stderr, "TODO 1: audio_out declares no latency: the model's own delay is %zu samples.\n",
                     k_model_latency);
        return 2;
    }

    print_report("model", processor.plan());
    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                latency,
                1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                k_host.m_sample_rate);

    // Offline, as a bounce: the host calls as fast as it can, and every block
    // waits for its result. Nothing may be missed, so the comparison is exact.
    std::vector<size_t> late;
    size_t position = 0;
    const std::vector<float> output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host.m_block_size,
        [&](float* samples, size_t num_samples) {
            if (processor.process(samples, num_samples, true) == ANIRA_MISSED) { late.push_back(position); }
            position += num_samples;
        });

    // Offline, the block has to come out processed, and nothing may be missed.
    if (std::equal(output.begin(), output.end(), k_input_signal.begin())) {
        std::fprintf(stderr, "TODO 3: offline, the output is the input: process the block with process_wait.\n");
        return 2;
    }
    if (!late.empty()) {
        std::fprintf(stderr, "TODO 3: offline, %zu blocks were missed: wait for the result, "
                             "process_wait with ANIRA_WAIT_FOREVER.\n", late.size());
        return 2;
    }

    // The reference already carries the model's own delay, so it lines up with
    // the output after the rest of the reported latency, as in step 7.
    const int result = report_with_latency(output.data(), k_target_output_signal.data(), output.size(),
                                           latency - k_model_latency, late, k_host.m_block_size);
    if (result != 0) { return result; }

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
        std::fprintf(stderr, "TODO 3: live, the output is the input: process the block in process().\n");
        return 2;
    }
    std::printf("OK: live, the blocks are processed too.\n");
    return 0;
} catch (const std::exception& error) {
    // Every refusal of anira's, from the files to prepare, names what it refused
    std::fprintf(stderr, "%s\n", error.what());
    return 2;
}
