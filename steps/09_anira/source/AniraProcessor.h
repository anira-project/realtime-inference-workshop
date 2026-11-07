// Step 9 — The model with anira: the processor.
//
// Everything anira does for the plugin, in one class: the model file and the
// contract say what runs and how, prepare plans it, process moves one block.
// The plugin (source/PluginProcessor.cpp) and the check (check.cpp) both drive it the
// way a host does: prepare once before the stream, process in every callback.
#pragma once

#include <anira/anira.hpp>

#include <cstddef>
#include <optional>
#include <string>

class AniraProcessor {
public:
    // Reads model.json and contract.json from `step_dir` and creates the
    // handler. Main thread.
    // @throws anira::Error with anira's reason
    explicit AniraProcessor(const std::string& step_dir);

    // Before the stream starts: the host's rate and its largest block. Plans
    // the handler and warms the model up. Main thread, blocking.
    // @throws anira::Error with anira's reason
    void prepare(double sample_rate, size_t max_block_size);

    // One callback, in place: one mono block in, the model's out. The model is
    // mono; the plugin mixes the host's channels down before and copies the
    // result out after.
    // @offline: the host renders offline, a bounce: wait for the result.
    //           Live (false): audio thread, no allocation, no lock, no wait.
    // @return anira's status: ANIRA_OK, or ANIRA_MISSED if the result was
    // not back in time and the miss policy filled the block
    anira_status process(float* samples, size_t num_samples, bool offline = false);

    // What the host is told, in samples. Valid after prepare.
    size_t latency() const { return m_latency; }
    // The budget prepare measured on this machine, in ms. Valid after prepare.
    double budget_ms() const { return m_budget_ms; }
    // The plan's row of the handler's report from the last prepare: budget,
    // measured and median time, warm-up, real-time factor and verdict. Empty
    // before a prepare. Its two strings live until the next prepare.
    anira_plan_info plan() const { return first_plan(m_handler->plan_report()); }

private:
    // In this order: the handlers run on the context and go before it
    anira::Context m_context{anira::ContextConfig{}};
    anira::ContractHandle m_contract;
    std::optional<anira::InferenceHandler> m_handler;  // Created in the constructor
    size_t m_latency = 0;
    double m_budget_ms = 0.0;

    static anira_plan_info first_plan(const anira::PlanReport& report) {
        return report.num_plans() > 0 ? report.plans().front() : anira_plan_info{};
    }
};
