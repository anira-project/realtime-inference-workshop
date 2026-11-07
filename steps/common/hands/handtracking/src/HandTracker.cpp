#include <handtracking/HandTracker.h>

#include "StreamedHandTracker.h"

namespace anira::models::handtracking {

std::unique_ptr<HandTracker> createHandTracker(const anira::ContextConfig& context_config) {
    return std::make_unique<StreamedHandTracker>(context_config);
}

}  // namespace anira::models::handtracking
