// Step 9 — The model with anira, reference implementation
//
// Goal:  the part 1 model on anira: the model file and the contract say what
//        runs and how, the handler does steps 2, 5 and 6.
// Given: model.json and contract.json next to this file, the plugin around it.
// Check: step09_solution_check: the output equals the reference, shifted by
//        latency().

#include "AniraProcessor.h"

AniraProcessor::AniraProcessor(const std::string& step_dir)
    : m_model(anira::ModelConfig::from_file(step_dir + "/model.json"))
    , m_contract(anira::ContractHandle::from_file(step_dir + "/contract.json")) {}

void AniraProcessor::prepare(double sample_rate, size_t max_block_size, size_t num_channels) {
    // ---- TODO 3 ------------------------------------------------------------
    // The main thread plans: the host's side of the contract, one handler per
    // channel over a pipeline with the model, prepare, the latency.
    m_contract.hard_host_blocks(1, max_block_size, sample_rate);  // Any block up to the host's largest

    m_handlers.clear();
    m_handlers.reserve(num_channels);
    if (num_channels == 0) { return; }
    for (size_t channel = 0; channel < num_channels; ++channel) {
        anira::Pipeline pipeline;
        pipeline.add_inference(m_model);
        m_handlers.emplace_back(m_context, pipeline);
        m_handlers.back().prepare(m_contract);  // Warm-up runs here, on this thread
    }
    m_latency = m_handlers.front().get_latency(0);  // Output slot 0: audio_out
    // ------------------------------------------------------------------------
}

size_t AniraProcessor::process(float* const* channels, size_t num_channels, size_t num_samples) {
    size_t missed = 0;
    for (size_t channel = 0; channel < num_channels && channel < m_handlers.size(); ++channel) {
        // One tensor over the host's block, [channels, samples]: no copy, no allocation
        const int64_t shape[2] = {1, static_cast<int64_t>(num_samples)};
        const anira::Tensor block = anira::Tensor::from_host(channels[channel], ANIRA_DTYPE_F32, shape);

        // ---- TODO 3: one call, in place ------------------------------------
        const anira_status status = m_handlers[channel].process(block, 0, block, 0);
        // --------------------------------------------------------------------
        if (status == ANIRA_MISSED) { ++missed; }  // The miss policy filled the block
    }
    return missed;
}
