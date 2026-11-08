// Step 10 — Opening the model, reference implementation
//
// Goal:  the model cut in the middle: encoder and decoder as two handlers, and
//        an offset per latent dimension between them.
// Given: encoder.json, decoder.json and contract.json next to this file, the
//        plugin around it.
// Check: step10_solution_check: with zero offsets the output equals step 10's
//        reference, shifted by latency(); an offset changes it.

#include "SplitProcessor.h"

#include "common/helpers/json_file.h"

namespace {

// One model as one handler, over a pipeline with one inference stage. The
// model file's path resolves against step_dir.
void create(std::optional<anira::InferenceHandler>& handler,
            anira::Context& context,
            const std::string& model_json,
            const std::string& step_dir) {
    anira::Pipeline pipeline;
    pipeline.add_inference(anira::ModelConfig::from_json(model_json, step_dir));
    handler.emplace(context, pipeline);
}

}  // namespace

SplitProcessor::SplitProcessor(const std::string& step_dir)
    : m_contract(anira::ContractHandle::from_json(read_json(step_dir + "/contract.json"))) {
    // Create: both model files, their // hints dropped, one handler each
    create(m_encoder, m_context, read_json(step_dir + "/encoder.json"), step_dir);
    create(m_decoder, m_context, read_json(step_dir + "/decoder.json"), step_dir);

    const int64_t frame_shape[2] = {k_num_latents, 1};
    m_frame = anira::Tensor::from_host(m_frame_data.data(), ANIRA_DTYPE_F32, frame_shape);
}

void SplitProcessor::prepare(double sample_rate, size_t max_block_size) {
    // Both handlers count in the host's audio samples, the encoder by its audio
    // input, the decoder by its audio output: the same host blocks and rate
    m_contract.hard_host_blocks(1, max_block_size, sample_rate);
    m_encoder->prepare(m_contract);
    m_decoder->prepare(m_contract);

    // What the host is told: the encoder's latency comes in latent frames,
    // k_frame_size samples each; the decoder's in samples
    m_encoder_latency_frames = m_encoder->get_latency();
    m_latency = m_encoder_latency_frames * k_frame_size + m_decoder->get_latency();

    // A new stream on both handlers: the count towards the next latent frame starts over
    m_input_samples = 0;
    m_frames = 0;
}

anira_status SplitProcessor::process(float* samples,
                                     size_t num_samples,
                                     const std::array<float, k_num_latents>& offsets,
                                     bool offline) {
    // One tensor over the host's block, [channels, samples]: no copy, no allocation
    const int64_t block_shape[2] = {1, static_cast<int64_t>(num_samples)};
    const anira::Tensor block = anira::Tensor::from_host(samples, ANIRA_DTYPE_F32, block_shape);

    // The encoder: audio in, and one latent frame out per k_frame_size input
    // samples, 21.5 a second
    m_input_samples += num_samples;
    m_encoder->push_data(block, 0);
    while (m_input_samples >= k_frame_size) {
        anira_status popped = ANIRA_OK;
        if (offline) {   // a bounce: wait for the frame
            popped = m_encoder->pop_data_wait(m_frame, 0, ANIRA_WAIT_FOREVER);
        } else {         // live: never waits, a late frame is a miss
            popped = m_encoder->pop_data(m_frame, 0);
        }
        if (popped != ANIRA_OK && popped != ANIRA_MISSED) { break; }  // A miss keeps its place
        m_input_samples -= k_frame_size;
        ++m_frames;

        // ---- TODO 2: an offset on z --------------------------------------
        // Each latent plus its dimension's offset
        for (size_t i = 0; i < k_num_latents; ++i) {
            m_frame_data[i] = m_frame_data[i] + offsets[i];
        }
        // ------------------------------------------------------------------

        // The decoder: the frame in
        m_decoder->push_data(m_frame, 0);
    }
    // The decoder: this callback's audio out
    anira_status status = ANIRA_OK;
    if (offline) {   // a bounce: wait for the audio
        status = m_decoder->pop_data_wait(block, 0, ANIRA_WAIT_FOREVER);
    } else {         // live: never waits
        status = m_decoder->pop_data(block, 0);
    }
    return status;  // ANIRA_MISSED: the miss policy filled the block
}
