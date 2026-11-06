#include <handtracking/HandTracker.h>

#include "StreamedHandTracker.h"
#include "SyncHandTracker.h"

namespace anira::models::handtracking {

std::unique_ptr<HandTracker> createHandTracker(Engine engine) {
    if (engine == Engine::Synchronous)
        return std::make_unique<SyncHandTracker>();
    return std::make_unique<StreamedHandTracker>();
}

}  // namespace anira::models::handtracking
