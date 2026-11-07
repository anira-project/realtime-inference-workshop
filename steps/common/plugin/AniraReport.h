// anira's plan report as rows for the plugin window, given complete: steps 9
// and 10 hand the rows of their handlers to PluginBase::set_report().
#pragma once

#include <anira/anira.hpp>

#include "PluginBase.h"

// One row of the report: what prepare measured for this model on this machine.
// @name: how the window labels it, "model", "encoder", "decoder"
// @plan: the plan's row of the handler's PlanReport
inline PluginBase::ReportRow report_row(const juce::String& name, const anira_plan_info& plan) {
    using Verdict = PluginBase::ReportRow::Verdict;
    PluginBase::ReportRow row;
    row.m_name = name;
    switch (plan.rt_verdict) {
        case ANIRA_RT_COMFORTABLE: row.m_verdict = Verdict::comfortable; break;
        case ANIRA_RT_MARGINAL: row.m_verdict = Verdict::marginal; break;
        case ANIRA_RT_NOT_REALTIME: row.m_verdict = Verdict::not_realtime; break;
        default: row.m_verdict = Verdict::unmeasured; break;
    }
    row.m_budget_ms = plan.budget_ms;
    row.m_measured_ms = plan.measured_ms;
    row.m_median_ms = plan.median_ms;
    row.m_warmup_inferences = plan.warmup_iterations;
    switch (plan.warmup_outcome) {
        case ANIRA_WARMUP_OUTCOME_FIXED: row.m_warmup = "a fixed count"; break;
        case ANIRA_WARMUP_OUTCOME_STABLE: row.m_warmup = "until stable"; break;
        case ANIRA_WARMUP_OUTCOME_CAPPED: row.m_warmup = "capped, it did not settle"; break;
        case ANIRA_WARMUP_OUTCOME_REUSED: row.m_warmup = "measured at an earlier prepare"; break;
        default: row.m_warmup = "no warm-up"; break;
    }
    row.m_rt_factor = plan.rt_factor;
    row.m_rt_parallel = plan.rt_parallel;
    return row;
}
