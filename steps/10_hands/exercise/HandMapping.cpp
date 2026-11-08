// Step 10 — The model as an instrument
//
// The last step, and a playground: no TODO, no check. Pick a mapping in
// map_hands at the bottom, rebuild, play. Change one, or write your own in
// yours(). Ten minutes, then we jam.
//
// Given: the camera and the hand tracking (common/hands), which hand leads
//        and the smoothing (HandControls), the decoder on anira (LatentDecoder).
//
// A mapping gets the lead hand (your right one) and, while there is one, the
// texture hand, about 60 times a second. Every feature runs from 0 to 1:
//   x         left to right        y      low to high
//   openness  fist to open palm    pinch  thumb and index apart to touching
//   roll      tilted left to right, 0.5 upright
// It returns the eight latents; about -2 to 2 is each one's natural range.
//   z0 loudness (+1 is about +5.5 dB, -3 close to silent)    z2 brightness
//   z1 body, z3 colour, z4 swell, z5 tilt, z6 dark/bright, z7 resonance: by ear
// map_range(feature, at_0, at_1): at_0 at feature 0, at_1 at feature 1.

#include "HandMapping.h"

#include <cmath>

namespace hands {

// 1 · The example: open is loud, high is bright, a second hand adds texture.
Latents classic(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
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

// 2 · Inverted: the fist is loud, low is bright.
Latents inverted(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
    Latents z = classic(lead, texture);
    z[0] = map_range(1.0F - lead.openness, -2.5F, 2.0F);
    z[2] = map_range(1.0F - lead.y, -2.5F, 3.0F);
    return z;
}

// 3 · Steps: the height in five fixed levels, like the notes of a scale.
Latents steps(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
    Latents z = classic(lead, texture);
    z[2] = map_range(std::round(lead.y * 4.0F) / 4.0F, -2.5F, 3.0F);
    return z;
}

// 4 · Two hands: the right one sets the loudness, the left one the brightness.
Latents two_hands(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
    Latents z{};
    z[0] = map_range(lead.openness, -2.5F, 2.0F);
    z[2] = texture.has_value() ? map_range(texture->y, -2.5F, 3.0F) : 0.0F;
    return z;
}

// 5 · Bow: thumb and index pinched together are the loudness, a curve keeps it quiet longer.
Latents bow(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
    Latents z = classic(lead, texture);
    z[0] = map_range(lead.pinch * lead.pinch, -3.0F, 2.0F);
    z[1] = 0.0F;
    return z;
}

// 6 · Yours: start from here.
Latents yours(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
    Latents z{};
    z[0] = map_range(lead.openness, -2.5F, 2.0F);
    (void)texture;  // the second hand, if you want it
    return z;
}

Latents map_hands(const HandFeatures& lead, const std::optional<HandFeatures>& texture) {
    return classic(lead, texture);  // <- or inverted, steps, two_hands, bow, yours
}

}  // namespace hands
