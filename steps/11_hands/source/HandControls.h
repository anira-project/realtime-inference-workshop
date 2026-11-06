// From the tracked hands to smoothed latents: which hand plays which role,
// map_hands (HandMapping.cpp, the playground), the smoothing. Given.
#pragma once

#include <array>
#include <optional>
#include <vector>

#include "HandMapping.h"
#include "HandTracker.h"
#include "LatentDecoder.h"

namespace hands {

// The lead hand plays (loudness, brightness, ...); the texture hand colours the sound.
enum class Role { Lead, Texture };
inline constexpr int k_num_roles = 2;

// For the panel: what each latent does in this model, and which hand drives it
// in the example mapping.
struct LatentInfo {
    Role role;
    const char* sound;
};
inline constexpr std::array<LatentInfo, k_num_latents> k_latent_info{{
    {Role::Lead, "loudness"},
    {Role::Lead, "body"},
    {Role::Lead, "brightness"},
    {Role::Lead, "colour"},
    {Role::Texture, "swell"},
    {Role::Lead, "tilt"},
    {Role::Texture, "dark/bright"},
    {Role::Texture, "resonance"},
}};

// Tracker result -> smoothed latents. Call from one thread (the GUI timer).
class HandControls {
public:
    struct Settings {
        float smoothing_ms = 80.0F;  // one-pole time constant while a hand is present
        float spread = 1.0F;         // widens or narrows every feature around its middle
        bool swap_hands = false;
    };

    // From the tracker: which hand plays which role, then updateFeatures().
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
    // Id of the tracked hand playing `role`, -1 if none (for the overlay).
    int handIdForRole(Role role) const { return m_hand_ids[static_cast<size_t>(role)]; }

    // Without a hand: z0 near silent, the rest in the middle.
    static constexpr Latents k_rest{-3.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};

private:
    static constexpr double k_hold_seconds = 0.2;    // bridges short tracking dropouts
    static constexpr double k_release_ms = 600.0;    // glide to rest after a hand leaves

    Latents m_latents = k_rest;
    Latents m_targets = k_rest;
    std::array<std::optional<HandFeatures>, k_num_roles> m_features;
    std::array<double, k_num_roles> m_missing_for{1e9, 1e9};
    std::array<bool, k_num_roles> m_present{false, false};
    std::array<int, k_num_roles> m_hand_ids{-1, -1};
};

}  // namespace hands
