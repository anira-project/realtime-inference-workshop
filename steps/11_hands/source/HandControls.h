// From a tracked hand to eight latents: features of the hand pose, one latent
// per feature, smoothed. The mapping table is the exercise.
#pragma once

#include <handtracking/HandTracker.h>

#include <array>
#include <optional>

#include "LatentDecoder.h"

namespace hands {

namespace ht = anira::models::handtracking;

// The lead hand plays (dynamics, register, ...); the texture hand colours the sound.
enum class Role { Lead, Texture };
inline constexpr int k_num_roles = 2;

enum class Feature { X, Y, Openness, Pinch, Roll };

// Hand pose features, each normalized to [0, 1] in mirrored (selfie) screen space.
struct HandFeatures {
    float x = 0.5F;         // 0 = left edge, 1 = right edge
    float y = 0.5F;         // 0 = bottom, 1 = top
    float openness = 0.0F;  // 0 = fist, 1 = open palm (fingertip-to-wrist distance / palm size)
    float pinch = 0.0F;     // 0 = thumb and index apart, 1 = touching
    float roll = 0.5F;      // 0 = tilted ~57 deg left, 0.5 = upright, 1 = tilted right
};

// One latent dimension driven by one hand feature, linearly from `at_0` to `at_1`.
struct LatentMapping {
    Role role;
    Feature feature;
    float at_0;
    float at_1;
    float rest;         // target while the role's hand is away
    const char* sound;  // what the dim does in this model
};

// Which feature drives which latent, and over what range: HandControls.cpp.
const std::array<LatentMapping, k_num_latents>& mappings();

// True while the table still has its TODO ranges (every range empty).
bool mappings_are_empty();

const char* role_name(Role role);
const char* feature_name(Feature feature);

// Features of one tracked hand; `aspect` is frame width / height (landmarks are normalized per axis).
HandFeatures compute_features(const ht::Hand& hand, float aspect);

// Which tracked hand plays which role: the right hand leads (left with `swap`); a single hand
// always leads; two hands with the same handedness are split by screen position.
std::array<const ht::Hand*, k_num_roles> assign_roles(const ht::Result& result, bool swap);

// Tracker result -> smoothed latents. Call from one thread (the GUI timer).
class HandControls {
public:
    struct Settings {
        float smoothing_ms = 80.0F;  // one-pole time constant while a hand is present
        float spread = 1.0F;         // scales each mapped range around its centre
        bool swap_hands = false;
    };

    // From the tracker: roles assigned, features computed, then updateFeatures().
    void update(const ht::Result& result, float aspect, double dt_seconds, const Settings& settings);

    // From anything that already has features: the pad, when there is no camera.
    void updateFeatures(const std::optional<HandFeatures>& lead,
                        const std::optional<HandFeatures>& texture,
                        double dt_seconds,
                        const Settings& settings);

    const Latents& latents() const { return m_latents; }
    const Latents& targets() const { return m_targets; }
    bool rolePresent(Role role) const { return m_present[static_cast<size_t>(role)]; }
    bool anyHandPresent() const { return m_present[0] || m_present[1]; }
    const std::optional<HandFeatures>& features(Role role) const { return m_features[static_cast<size_t>(role)]; }
    // Tracker slot of the hand playing `role`, -1 if none (for the overlay).
    int slotForRole(Role role) const { return m_slots[static_cast<size_t>(role)]; }

private:
    static constexpr double k_hold_seconds = 0.2;    // bridges short tracking dropouts
    static constexpr double k_release_ms = 600.0;    // glide to rest after a hand leaves

    Latents m_latents = rest_latents();
    Latents m_targets = rest_latents();
    std::array<std::optional<HandFeatures>, k_num_roles> m_features;
    std::array<double, k_num_roles> m_missing_for{1e9, 1e9};
    std::array<bool, k_num_roles> m_present{false, false};
    std::array<int, k_num_roles> m_slots{-1, -1};

    static Latents rest_latents();
};

}  // namespace hands
