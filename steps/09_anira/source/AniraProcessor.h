// Step 9 — The model with anira: the processor.
//
// Everything anira does for the plugin, in one class: the model file and the
// contract say what runs and how, prepare plans it, process moves one block.
// The plugin (source/Plugin.cpp) and the check (check.cpp) both drive it the
// way a host does: prepare once before the stream, process in every callback.
#pragma once

#include <anira/anira.hpp>

#include <cstddef>
#include <optional>
#include <string>

class AniraProcessor {
public:
    // Reads model.json and contract.json from `step_dir`. Main thread.
    // @throws anira::Error with anira's reason
    explicit AniraProcessor(const std::string& step_dir);

    // Before the stream starts: the host's rate and its largest block. Plans
    // the handler and warms the model up. Main thread, blocking.
    // @throws anira::Error with anira's reason
    void prepare(double sample_rate, size_t max_block_size);

    // One callback, in place: one mono block in, the model's out. The model is
    // mono; the plugin mixes the host's channels down before and copies the
    // result out after. Audio thread: no allocation, no lock, no wait.
    // @return true if the miss policy filled the block
    bool process(float* samples, size_t num_samples);

    // What the host is told, in samples. Valid after prepare.
    size_t latency() const { return m_latency; }

private:
    // In this order: the handlers run on the context and go before it
    anira::Context m_context{anira::ContextConfig{}};
    anira::ModelConfig m_model;
    anira::ContractHandle m_contract;
    std::optional<anira::InferenceHandler> m_handler;  // Created at the first prepare
    size_t m_latency = 0;
};
