// Step 9 — The model with anira
//
// Goal:   the part 1 model on anira: the model file and the contract say what
//         runs and how, the handler does steps 3, 6 and 7.
// Given:  contract.json, the plugin around this class, anira (anira.hpp).
// Task:   TODO 1 and 2 in model.json: replace every ? there.
//         TODO 3 below: four blanks, marked "fill out".
// Check:  step09_exercise_check: the output equals the reference, shifted by
//         latency(). Then listen: the plugin step09_exercise.
//
// It builds once every ? is filled in, then the check says what is still off.

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
    // The main thread plans: the host's side of the contract, prepare, the
    // latency. Two lines are missing:
    //   - the host's side of the contract: its smallest block (any block may
    //     come, down to 1), its largest (max_block_size) and its sample rate.
    //   - the latency of output slot 0 (audio_out), from the prepared handler:
    //     handler.get_latency(slot).
    //
    m_contract.hard_host_blocks(?, ?, ?);  // <- 1. fill out

    const anira::PlanReport report = m_handler->prepare(m_contract);  // Warm-up and measurement run here
    m_budget_ms = report.plans().front().measured_ms;                  // This machine's
    m_latency = ?;  // <- 2. fill out
    // ------------------------------------------------------------------------
}

anira_status AniraProcessor::process(float* samples, size_t num_samples, bool offline) {
    // One tensor over the host's block, [channels, samples]: no copy, no allocation
    const int64_t shape[2] = {1, static_cast<int64_t>(num_samples)};
    const anira::Tensor block = anira::Tensor::from_host(samples, ANIRA_DTYPE_F32, shape);

    // ---- TODO 3: one call, in place ----------------------------------------
    //   Two cases, the same values in both, and we have to handle each:
    //   - live, the audio thread: m_handler->process(input, input slot,
    //     output, output slot). It never waits; a late block is a miss.
    //   - offline, a bounce: m_handler->process_wait(the same four, timeout).
    //     ANIRA_WAIT_FOREVER waits as long as the inference takes: a bounce
    //     has no deadline, so no block is missed.
    //   The block is input and output, and slot 0 is the audio.
    // ------------------------------------------------------------------------
    anira_status status = ANIRA_OK;
    if (offline) {   // a bounce: no deadline, wait for the result
        status = m_handler->process_wait(?, 0, ?, 0, ANIRA_WAIT_FOREVER);  // <- 3. fill out
    } else {         // live: never waits
        status = m_handler->process(?, 0, ?, 0);  // <- 4. fill out
    }
    return status;  // ANIRA_MISSED: the miss policy filled the block
}
