// Step 10 — The model as an instrument
//
// The fun part: everything is in place, no TODO, no check. One mapping from
// the hands to the latent vector; change it, rebuild, play.
//
// Given: the camera and the hand tracking (common/hands), which hand leads
//        and the smoothing (HandControls), the decoder on anira (LatentDecoder).
//
// map_hands gets the lead hand (your right one) and, while there is one, the
// texture hand, about 60 times a second. Every feature runs from 0 to 1:
//   x         left to right        y      low to high
//   openness  fist to open palm    pinch  thumb and index apart to touching
//   roll      tilted left to right, 0.5 upright
// It returns the latent vector, eight dimensions; about -2 to 2 is each one's
// natural range.
//   z0 loudness (+1 is about +5.5 dB, -3 close to silent)    z2 brightness
//   z1 body, z3 colour, z4 swell, z5 tilt, z6 dark/bright, z7 resonance: by ear
// map_range(feature, at_0, at_1): at_0 at feature 0, at_1 at feature 1.

#include "HandMapping.h"

namespace hands {

// Open is loud, high is bright, a second hand adds texture.
Latents map_hands(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
    Latents z{};
    z[0] = map_range(lead.openness, -2.5F, 2.0F);  // loudness: a fist is quiet
    z[1] = map_range(lead.pinch, 0.5F, -3.0F);     // body: a pinch, darker and louder
    z[2] = map_range(lead.y, -2.5F, 3.0F);         // brightness: low dark, high bright
    z[3] = map_range(lead.x, -2.5F, 2.5F);         // colour
    z[5] = map_range(lead.roll, -3.0F, 3.0F);      // tilt
    if (texture.has_value()) {
        z[4] = map_range(texture->y, -2.5F, 3.0F);        // swell
        z[6] = map_range(texture->x, -3.0F, 3.0F);        // dark/bright
        z[7] = map_range(texture->openness, 0.0F, 3.0F);  // resonance
    }
    return z;
}

}  // namespace hands
