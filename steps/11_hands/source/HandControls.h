// From a tracked hand to eight latents: features of the hand pose, one latent
// per feature, smoothed. The mapping table is the exercise.
#pragma once

#include <array>
#include <optional>
#include <vector>

#include "HandTracker.h"
#include "LatentDecoder.h"

namespace hands {

// The lead hand plays (dynamics, register, ...); the texture hand colours the sound.
enum class Role { Lead, Texture };
inline constexpr int k_num_roles = 2;

enum class Feature { X, Y, Openness, Pinch, Roll };

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

// True while the table still has its TODO 2 ranges (every range empty).
bool mappings_are_empty();

const char* role_name(Role role);
const char* feature_name(Feature feature);

// Tracker result -> smoothed latents. Call from one thread (the GUI timer).
class HandControls {
public:
    struct Settings {
        float smoothing_ms = 80.0F;  // one-pole time constant while a hand is present
        float spread = 1.0F;         // scales each mapped range around its centre
        bool swap_hands = false;
    };

    // From the tracker: which hand plays which role (TODO 1), then updateFeatures().
    void update(const std::vector<Hand>& hands, double dt_seconds, const Settings& settings);

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
    // Id of the tracked hand playing `role`, -1 if none (for the overlay).
    int handIdForRole(Role role) const { return m_hand_ids[static_cast<size_t>(role)]; }

private:
    static constexpr double k_hold_seconds = 0.2;    // bridges short tracking dropouts
    static constexpr double k_release_ms = 600.0;    // glide to rest after a hand leaves

    Latents m_latents = rest_latents();
    Latents m_targets = rest_latents();
    std::array<std::optional<HandFeatures>, k_num_roles> m_features;
    std::array<double, k_num_roles> m_missing_for{1e9, 1e9};
    std::array<bool, k_num_roles> m_present{false, false};
    std::array<int, k_num_roles> m_hand_ids{-1, -1};

    static Latents rest_latents();
};

}  // namespace hands
