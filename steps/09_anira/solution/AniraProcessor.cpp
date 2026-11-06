// Step 9 — The model with anira, reference implementation
//
// Goal:  the part 1 model on anira: the model file and the contract say what
//        runs and how, the handler does steps 3, 6 and 7.
// Given: model.json and contract.json next to this file, the plugin around it.
// Check: step09_solution_check: the output equals the reference, shifted by
//        latency().

#include "AniraProcessor.h"

#include "common/helpers/json_file.h"

AniraProcessor::AniraProcessor(const std::string& step_dir)
    : m_contract(anira::ContractHandle::from_json(read_json(step_dir + "/contract.json"))) {
    // The model file as JSON text, its // hints dropped
    const std::string model_json = read_json(step_dir + "/model.json");

    // Create: the model in a pipeline, the handler over it. It checks the
    // files and builds the slots; it loads nothing yet. The model's path in
    // the file resolves against step_dir.
    anira::Pipeline pipeline;
    pipeline.add_inference(anira::ModelConfig::from_json(model_json, step_dir));
    m_handler.emplace(m_context, pipeline);
}

void AniraProcessor::prepare(double sample_rate, size_t max_block_size) {
    // ---- TODO 3 ------------------------------------------------------------
    // The main thread plans: the host's side of the contract, prepare, the latency.
    m_contract.hard_host_blocks(1, max_block_size, sample_rate);  // Any block up to the host's largest

    const anira::PlanReport report = m_handler->prepare(m_contract);  // Warm-up and measurement run here
    m_budget_ms = report.plans().front().measured_ms;                  // This machine's
    m_latency = m_handler->get_latency(0);       // Output slot 0: audio_out
    // ------------------------------------------------------------------------
}

anira_status AniraProcessor::process(float* samples, size_t num_samples, bool offline) {
    // One tensor over the host's block, [channels, samples]: no copy, no allocation
    const int64_t shape[2] = {1, static_cast<int64_t>(num_samples)};
    const anira::Tensor block = anira::Tensor::from_host(samples, ANIRA_DTYPE_F32, shape);

    // ---- TODO 3: one call, in place ----------------------------------------
    anira_status status = ANIRA_OK;
    if (offline) {   // a bounce: no deadline, wait for the result
        status = m_handler->process_wait(block, 0, block, 0, ANIRA_WAIT_FOREVER);
    } else {         // live: never waits
        status = m_handler->process(block, 0, block, 0);
    }
    return status;  // ANIRA_MISSED: the miss policy filled the block
    // ------------------------------------------------------------------------
}
