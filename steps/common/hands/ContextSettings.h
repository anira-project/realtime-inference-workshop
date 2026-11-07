// The one anira context configuration of step 11: the hand tracker and the decoder each make
// their anira::Context from it. anira runs every context of a process on one core with one
// thread pool, so they must ask for the same settings; otherwise the first one wins and every
// other one is warned about it.
#pragma once

#include <algorithm>
#include <anira/anira.hpp>
#include <thread>

namespace hands {

inline anira::ContextConfig context_config() {
    anira::ContextConfig config;
    // One thread per core, shared by the camera's models and the decoder. Blocking: the threads
    // sleep between camera frames instead of spinning.
    config.threads(std::max(1U, std::thread::hardware_concurrency()), ANIRA_WAIT_BLOCKING);
    // Errors only: at every start ONNX Runtime warns about the GPUs it cannot probe and anira
    // about the gesture stages' generous budgets, neither of which concerns this app.
    config.log_level(ANIRA_LOG_ERROR);
    return config;
}

}  // namespace hands
