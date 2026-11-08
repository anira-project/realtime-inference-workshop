// From the hands to the eight latents: the one function of this step's playground.
#pragma once

#include <optional>

#include "HandTracker.h"
#include "LatentDecoder.h"

namespace hands {

// The straight line from at_0 (feature 0) to at_1 (feature 1).
inline float map_range(float feature, float at_0, float at_1) {
    return at_0 + (at_1 - at_0) * feature;
}

// The lead hand's features, and the texture hand's while a second hand is
// there; every feature from 0 to 1 (HandTracker.h). Returns the eight
// latents; HandControls smooths them for the decoder. HandMapping.cpp.
Latents map_hands(const HandFeatures& lead, const std::optional<HandFeatures>& texture);

}  // namespace hands
