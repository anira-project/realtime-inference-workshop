// Step 9 — The model with anira: the processor.
//
// Everything anira does for the plugin, in one class: the model file and the
// contract say what runs and how, prepare plans it, process moves one block.
// The plugin (source/Plugin.cpp) and the check (check.cpp) both drive it the
// way a host does: prepare once before the stream, process in every callback.
#pragma once

#include <anira/anira.hpp>

#include <cstddef>
#include <string>
#include <vector>

class AniraProcessor {
public:
    // Reads model.json and contract.json from `step_dir`. Main thread.
    // @throws anira::Error with anira's reason
    explicit AniraProcessor(const std::string& step_dir);

    // Before the stream starts: the host's rate, its largest block and its
    // channels. Plans one handler per channel and warms the model up. Main
    // thread, blocking.
    // @throws anira::Error with anira's reason
    void prepare(double sample_rate, size_t max_block_size, size_t num_channels);

    // One callback, in place: every channel's block in, the model's out.
    // Audio thread: no allocation, no lock, no wait.
    // @return how many channels the miss policy filled (0: all on time)
    size_t process(float* const* channels, size_t num_channels, size_t num_samples);

    // What the host is told, in samples. Valid after prepare.
    size_t latency() const { return m_latency; }

private:
    // In this order: the handlers run on the context and go before it
    anira::Context m_context{anira::ContextConfig{}};
    anira::ModelConfig m_model;
    anira::ContractHandle m_contract;
    std::vector<anira::InferenceHandler> m_handlers;  // One per channel: the model is mono, its state per signal
    size_t m_latency = 0;
};
