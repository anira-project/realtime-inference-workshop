// Step 9 — The model with anira, reference implementation
//
// Goal:  the part 1 model on anira: the model file and the contract say what
//        runs and how, the handler does steps 2, 5 and 6.
// Given: model.json and contract.json next to this step, the fake host.
// Check: the output equals the reference, shifted by get_latency().

#include <anira/anira.hpp>

#include <cstddef>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "common/helpers/host.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

// The host side of the contract: what the fake host does, in samples at its rate.
constexpr struct {
    size_t m_block_size = 512;
    double m_sample_rate = 44100.0;
} k_host{};

// The model's own delay, as declared in model.json. The reference output
// already carries it, and so does the latency anira reports.
constexpr size_t k_model_latency = 2224;

int main() try {
    const std::string step_dir = WORKSHOP_STEP_DIR;

    // ---- TODO 3 ------------------------------------------------------------
    // The main thread plans: the two files, a pipeline over the model, the
    // handler, prepare.
    const anira::ModelConfig model = anira::ModelConfig::from_file(step_dir + "/model.json");
    anira::ContractHandle contract = anira::ContractHandle::from_file(step_dir + "/contract.json");
    contract.hard_host_blocks(k_host.m_block_size, k_host.m_block_size, k_host.m_sample_rate);

    anira::Context context{anira::ContextConfig{}};
    anira::Pipeline pipeline;
    pipeline.add_inference(model);
    anira::InferenceHandler handler(context, pipeline);

    handler.prepare(contract);                      // Warm-up runs here, on this thread
    const size_t latency = handler.get_latency(0);  // Output slot 0: audio_out
    // ------------------------------------------------------------------------

    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                latency,
                1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                k_host.m_sample_rate);

    std::vector<size_t> late;  // Where a block that was not back in time went out as silence
    size_t position = 0;

    // The audio thread moves samples: in place, one call, never waits.
    const std::vector<float> output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host.m_block_size,
        [&](float* samples, size_t num_samples) {
            // One tensor over the host's block, [channels, samples]: no copy, no allocation
            const int64_t shape[2] = {1, static_cast<int64_t>(num_samples)};
            const anira::Tensor block = anira::Tensor::from_host(samples, ANIRA_DTYPE_F32, shape);
            const anira_status status = handler.process(block, 0, block, 0);
            if (status == ANIRA_MISSED) { late.push_back(position); }  // The miss policy filled the block
            position += num_samples;
        },
        k_host.m_sample_rate);

    // The reference already carries the model's own delay, so it lines up with
    // the output after the rest of the reported latency, as in step 6.
    return report_with_latency(output.data(), k_target_output_signal.data(), output.size(),
                               latency - k_model_latency, late, k_host.m_block_size);
} catch (const std::exception& error) {
    // Every refusal of anira's, from the files to prepare, names what it refused
    std::fprintf(stderr, "%s\n", error.what());
    return 2;
}
