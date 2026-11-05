// Step 10 — Encoder, latents, decoder, reference implementation
//
// Goal:  the model cut in the middle: two handlers on one context, and an
//        offset per latent dimension between them.
// Given: encoder.json, decoder.json and contract.json next to this file, the
//        plugin around it.
// Check: step10_solution_check: with zero offsets the output equals step 10's
//        reference, shifted by latency(); an offset changes it.

#include "SplitProcessor.h"

namespace {

// One model as one handler, over a pipeline with one inference stage.
void create(std::optional<anira::InferenceHandler>& handler,
            anira::Context& context,
            const anira::ModelConfig& model) {
    anira::Pipeline pipeline;
    pipeline.add_inference(model);
    handler.emplace(context, pipeline);
}

}  // namespace

SplitProcessor::SplitProcessor(const std::string& step_dir)
    : m_encoder_model(anira::ModelConfig::from_file(step_dir + "/encoder.json"))
    , m_decoder_model(anira::ModelConfig::from_file(step_dir + "/decoder.json"))
    , m_contract(anira::ContractHandle::from_file(step_dir + "/contract.json")) {
    const int64_t frame_shape[2] = {k_num_latents, 1};
    m_frame = anira::Tensor::from_host(m_frame_data.data(), ANIRA_DTYPE_F32, frame_shape);
}

void SplitProcessor::prepare(double sample_rate, size_t max_block_size) {
    // Both handlers on the same contract: any host block up to the largest
    m_contract.hard_host_blocks(1, max_block_size, sample_rate);
    if (!m_encoder) {
        create(m_encoder, m_context, m_encoder_model);
        create(m_decoder, m_context, m_decoder_model);
    }
    m_encoder->prepare(m_contract);
    m_decoder->prepare(m_contract);
    m_encoder_latency_frames = m_encoder->get_latency(0);       // In latent frames
    const size_t decoder_latency = m_decoder->get_latency(0);   // In samples

    // ---- TODO 1 ------------------------------------------------------------
    // The latency the host is told, in samples. One latent frame is
    // k_frame_size samples.
    // ------------------------------------------------------------------------
    m_latency = m_encoder_latency_frames * k_frame_size + decoder_latency;

    // A new stream: the count towards the next latent frame starts over
    m_input_samples = 0;
    m_frames = 0;
}

bool SplitProcessor::process(float* samples, size_t num_samples) {
    // One tensor over the host's block, [channels, samples]: no copy, no allocation
    const int64_t block_shape[2] = {1, static_cast<int64_t>(num_samples)};
    const anira::Tensor block = anira::Tensor::from_host(samples, ANIRA_DTYPE_F32, block_shape);

    // ---- TODO 2 ------------------------------------------------------------
    // The encoder: the block in, and one latent frame out into m_frame per
    // k_frame_size input samples.
    // ------------------------------------------------------------------------
    m_input_samples += num_samples;
    m_encoder->push_data(block, 0);
    while (m_input_samples >= k_frame_size) {
        const anira_status popped = m_encoder->pop_data(m_frame, 0);
        if (popped != ANIRA_OK && popped != ANIRA_MISSED) { break; }  // A missed frame keeps its place
        m_input_samples -= k_frame_size;
        ++m_frames;

        // ---- TODO 3 --------------------------------------------------------
        // An offset on z: each dimension's knob, m_offset[i], on its latent.
        // --------------------------------------------------------------------
        for (size_t i = 0; i < k_num_latents; ++i) {
            m_frame_data[i] += m_offset[i].load();
        }

        // ---- TODO 4 --------------------------------------------------------
        // The decoder: every frame in, and below, the audio for this callback
        // out into the block.
        // --------------------------------------------------------------------
        m_decoder->push_data(m_frame, 0);
    }
    const anira_status status = m_decoder->pop_data(block, 0);
    return status == ANIRA_MISSED;  // The miss policy filled the block
}
