// Which TODO a failure points to when its message names none: the engine's
// and anira's errors, and an output that does not match. The result line
// names it, so a participant knows where to look. Given complete.
#pragma once

#include <string>

// The bonus step has no number: its checks pass this one.
inline constexpr int k_bonus = 0;

// The TODO behind a FAIL message of `step`, "TODO 1", "TODO 2 or 3", or
// empty when it is not a TODO (the worker was late, say).
// @step: the step's number
// @message: what FAIL said
inline std::string todo_for(int step, const std::string& message) {
    auto has = [&](const char* part) { return message.find(part) != std::string::npos; };
    switch (step) {
    case 1:
        if (has("cannot load")) { return "TODO 1"; }
        return "TODO 2";  // The size, or where the block starts
    case 2:
        return "TODO 2";  // check_state() names TODO 1: the rest is what is passed
    case 3:
        return "TODO 2 or 3";  // Samples lost between the host, the model and back
    case 6:
        if (has("dropout")) { return {}; }  // The worker was late: k_max_inference_ms
        return "TODO 2 or 3";
    case 9:
        if (has("state")) { return "TODO 2"; }  // anira refused the state pair
        if (has("'audio")) { return "TODO 1"; }  // anira refused audio_in or audio_out
        return "TODO 1 or 3";  // The latency in model.json, or the calls
    case k_bonus:
        if (has("missed")) { return {}; }  // Offline nothing is missed: not a TODO
        if (has("does not match")) { return "TODO 2"; }  // prepare took the files: the offset line
        return "TODO 1";  // anira refused a tensor in encoder.json or decoder.json
    default:
        return {};
    }
}
