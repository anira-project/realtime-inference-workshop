// Step 9 — The model with anira
//
// Goal:   the part 1 model on anira: the model file and the contract say what
//         runs and how, the handler does steps 3, 6 and 7.
// Given:  contract.json, the plugin around this class, anira (anira.hpp).
// Task:   TODO 1 and 2 in model.json: replace every "TODO n" there.
//         TODO 3 below: three blanks, marked "fill out".
// Check:  step09_exercise_check: the output equals the reference, shifted by
//         latency(). Then listen: the plugin step09_exercise.
//
// It builds and runs as it is, and the check says what is still missing.

#include "AniraProcessor.h"

AniraProcessor::AniraProcessor(const std::string& step_dir)
    : m_model(anira::ModelConfig::from_file(step_dir + "/model.json"))
    , m_contract(anira::ContractHandle::from_file(step_dir + "/contract.json")) {}

void AniraProcessor::prepare(double sample_rate, size_t max_block_size) {
    // ---- TODO 3 ------------------------------------------------------------
    // The main thread plans: the host's side of the contract, the handler over
    // a pipeline with the model, prepare, the latency. Two lines are missing:
    //   - the host's side of the contract: its smallest block (any block may
    //     come, down to 1), its largest (max_block_size) and its sample rate.
    //   - the latency of output slot 0 (audio_out), from the prepared handler:
    //     handler.get_latency(slot).
    // m_contract.hard_host_blocks(?, ?, ?); // <- 1. fill out

    if (!m_handler) {
        anira::Pipeline pipeline;
        pipeline.add_inference(m_model);
        m_handler.emplace(m_context, pipeline);
    }
    m_handler->prepare(m_contract);              // Warm-up runs here, on this thread
    m_latency = 0; // <- 2. fill out
    // ------------------------------------------------------------------------
}

bool AniraProcessor::process(float* samples, size_t num_samples) {
    // One tensor over the host's block, [channels, samples]: no copy, no allocation
    const int64_t shape[2] = {1, static_cast<int64_t>(num_samples)};
    const anira::Tensor block = anira::Tensor::from_host(samples, ANIRA_DTYPE_F32, shape);

    // ---- TODO 3: one call, in place ----------------------------------------
    //   m_handler->process(input, input slot, output, output slot): the block
    //   is both, and slot 0 is the audio.
    // ------------------------------------------------------------------------
    anira_status status = ANIRA_OK;
    // status = m_handler->process(?, 0, ?, 0); // <- 3. fill out
    return status == ANIRA_MISSED;  // The miss policy filled the block
}
