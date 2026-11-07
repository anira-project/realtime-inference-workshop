// anira's plan report in the terminal, given complete: the checks of steps 9
// and 10 print it after prepare, the same figures the plugin's report shows.
#pragma once

#include <anira/anira.hpp>

#include <cstdio>

// One model's row: what prepare measured on this machine.
// @name: "model", "encoder", "decoder"
// @plan: the plan's row of the handler's PlanReport
inline void print_report(const char* name, const anira_plan_info& plan) {
    const char* verdict = plan.rt_verdict == ANIRA_RT_COMFORTABLE    ? "comfortable"
                          : plan.rt_verdict == ANIRA_RT_MARGINAL     ? "marginal: may drop out under load"
                          : plan.rt_verdict == ANIRA_RT_NOT_REALTIME ? "NOT real-time: live, it drops out"
                                                                     : "unmeasured";
    const char* warmup = plan.warmup_outcome == ANIRA_WARMUP_OUTCOME_STABLE   ? "until stable"
                         : plan.warmup_outcome == ANIRA_WARMUP_OUTCOME_CAPPED ? "capped, it did not settle"
                         : plan.warmup_outcome == ANIRA_WARMUP_OUTCOME_FIXED  ? "a fixed count"
                                                                              : "none";
    // The real-time factor shared by the inferences that run at once: below
    // 0.75 comfortable, below 1 marginal, else not real-time
    const double load = plan.rt_parallel > 0 ? plan.rt_factor / plan.rt_parallel : plan.rt_factor;
    std::printf("anira's report, %s (at prepare, on this machine):\n", name);
    std::printf("  budget %.1f ms   measured %.1f ms   median %.1f ms\n",
                plan.budget_ms,
                plan.measured_ms,
                plan.median_ms);
    std::printf("  warm-up %u inferences, %s\n", plan.warmup_iterations, warmup);
    std::printf("  load %.0f %% (real-time factor %.2f over %u %s): %s\n\n",
                load * 100.0,
                plan.rt_factor,
                plan.rt_parallel,
                plan.rt_parallel == 1 ? "inference" : "inferences",
                verdict);
}
