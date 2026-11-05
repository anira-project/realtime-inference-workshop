// Step 11 — Hands on the latents
//
// Goal:   which feature of the hand drives which latent, and over what range.
// Given:  the hands and their features (HandTracker.h), the roles, the smoothing.
// Task:   TODO 1, which tracked hand plays which role; TODO 2, the table.
//
// It builds and runs as it is: no hand plays, every latent stays at rest.

#include "HandControls.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hands {

namespace {

// Derived from the latent analysis of the model (anira-webcam-rave-demo/tools/latent-analysis):
// the natural range of each dimension is about +-2; z0 sets the energy (-3 is close to
// silent), z2 tracks register and brightness, z4 and z1 add a darker, louder body; z6 and z7
// grow louder away from 0 in both directions, so they map one-sided or across the full width.
// ---- TODO 2 ------------------------------------------------------------------
// One row per latent: which hand (Lead or Texture), which feature (X, Y,
// Openness, Pinch, Roll), the latent at feature 0 and at feature 1, and the
// value at rest. Every range is empty below, so the hand changes nothing.
// Start with z0 (dynamics: fist quiet, open loud) and z2 (register: low to
// high), then play with the rest.
constexpr std::array<LatentMapping, k_num_latents> k_mappings{{
    {Role::Lead, Feature::Openness, 0.0F, 0.0F, -3.0F, "dynamics"},
    {Role::Lead, Feature::Pinch, 0.0F, 0.0F, 0.0F, "body"},
    {Role::Lead, Feature::Y, 0.0F, 0.0F, 0.0F, "register"},
    {Role::Lead, Feature::X, 0.0F, 0.0F, 0.0F, "colour"},
    {Role::Texture, Feature::Y, 0.0F, 0.0F, 0.0F, "swell"},
    {Role::Lead, Feature::Roll, 0.0F, 0.0F, 0.0F, "tilt"},
    {Role::Texture, Feature::X, 0.0F, 0.0F, 0.0F, "dark/bright"},
    {Role::Texture, Feature::Openness, 0.0F, 0.0F, 0.0F, "resonance"},
}};
// ------------------------------------------------------------------------------

float feature_value(const HandFeatures& features, Feature feature) {
    switch (feature) {
        case Feature::X: return features.x;
        case Feature::Y: return features.y;
        case Feature::Openness: return features.openness;
        case Feature::Pinch: return features.pinch;
        case Feature::Roll: return features.roll;
    }
    return 0.0F;
}

}  // namespace

const std::array<LatentMapping, k_num_latents>& mappings() {
    return k_mappings;
}

bool mappings_are_empty() {
    for (const auto& mapping : k_mappings) {
        if (mapping.at_0 != mapping.at_1) { return false; }
    }
    return true;
}

const char* role_name(Role role) {
    return role == Role::Lead ? "lead" : "texture";
}

const char* feature_name(Feature feature) {
    switch (feature) {
        case Feature::X: return "left-right";
        case Feature::Y: return "height";
        case Feature::Openness: return "open";
        case Feature::Pinch: return "pinch";
        case Feature::Roll: return "tilt";
    }
    return "";
}

Latents HandControls::rest_latents() {
    Latents rest{};
    for (size_t i = 0; i < rest.size(); ++i)
        rest[i] = mappings()[i].rest;
    return rest;
}

void HandControls::update(const std::vector<Hand>& hands, double dt_seconds, const Settings& settings) {
    const Hand* lead = nullptr;     // plays: your right hand (the left with swap_hands)
    const Hand* texture = nullptr;  // colours: the other hand

    // ---- TODO 1 ------------------------------------------------------------------
    // `hands` is what HandTracker::hands() returned (HandTracker.h): up to two
    // hands, the most confident first. Point `lead` and `texture` at them:
    // the hand with is_right != settings.swap_hands leads, the other one colours,
    // and a single hand always leads. Both stay nullptr for a missing hand.

    // ------------------------------------------------------------------------------

    m_hand_ids = {lead != nullptr ? lead->id : -1, texture != nullptr ? texture->id : -1};
    updateFeatures(lead != nullptr ? std::optional(lead->features) : std::nullopt,
                   texture != nullptr ? std::optional(texture->features) : std::nullopt,
                   dt_seconds, settings);
}

void HandControls::updateFeatures(const std::optional<HandFeatures>& lead,
                                  const std::optional<HandFeatures>& texture,
                                  double dt_seconds,
                                  const Settings& settings) {
    const std::array<const std::optional<HandFeatures>*, k_num_roles> incoming{&lead, &texture};
    for (size_t role = 0; role < k_num_roles; ++role) {
        if (incoming[role]->has_value()) {
            m_features[role] = **incoming[role];
            m_missing_for[role] = 0.0;
        } else {
            m_missing_for[role] += dt_seconds;
        }
        m_present[role] = m_missing_for[role] < k_hold_seconds;
        if (!m_present[role])
            m_features[role].reset();
    }

    for (size_t i = 0; i < k_num_latents; ++i) {
        const auto& mapping = mappings()[i];
        const auto role = static_cast<size_t>(mapping.role);
        const auto& features = m_features[role];

        if (features.has_value()) {
            const float centre = 0.5F * (mapping.at_0 + mapping.at_1);
            const float value = mapping.at_0 + (mapping.at_1 - mapping.at_0) * feature_value(*features, mapping.feature);
            m_targets[i] = centre + (value - centre) * settings.spread;
        } else {
            m_targets[i] = mapping.rest;
        }

        const double tau_ms = features.has_value() ? settings.smoothing_ms : k_release_ms;
        const auto coefficient = tau_ms <= 0.0 ? 1.0F : static_cast<float>(1.0 - std::exp(-dt_seconds * 1000.0 / tau_ms));
        m_latents[i] += (m_targets[i] - m_latents[i]) * coefficient;
    }
}

}  // namespace hands
