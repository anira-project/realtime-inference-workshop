// Step 11 — Hands on the latents, reference implementation
//
// Goal:  which feature of the hand drives which latent, and over what range.
// Given: the features (compute_features), the roles, the smoothing.

#include "HandControls.h"

#include <algorithm>
#include <cmath>

namespace hands {

namespace {

// MediaPipe hand landmark indices.
constexpr int k_wrist = 0;
constexpr int k_thumb_tip = 4;
constexpr int k_index_mcp = 5;
constexpr int k_index_tip = 8;
constexpr int k_middle_mcp = 9;
constexpr int k_middle_tip = 12;
constexpr int k_ring_mcp = 13;
constexpr int k_ring_tip = 16;
constexpr int k_pinky_mcp = 17;
constexpr int k_pinky_tip = 20;

// Pose ratios (relative to palm length) mapped onto [0, 1].
constexpr float k_fist_ratio = 1.0F;   // mean fingertip-to-wrist distance, closed fist
constexpr float k_open_ratio = 1.75F;  // ... open palm
constexpr float k_pinch_closed_ratio = 0.2F;  // thumb tip to index tip, touching
constexpr float k_pinch_open_ratio = 0.8F;
constexpr float k_roll_range = 1.0F;   // radians each side of upright
constexpr float k_edge_margin = 0.15F; // x/y reach the ends this far inside the frame

// Derived from the latent analysis of the model (anira-webcam-rave-demo/tools/latent-analysis):
// the natural range of each dimension is about +-2; z0 sets the energy (-3 is close to
// silent), z2 tracks register and brightness, z4 and z1 add a darker, louder body; z6 and z7
// grow louder away from 0 in both directions, so they map one-sided or across the full width.
// ---- TODO 1 ------------------------------------------------------------------
constexpr std::array<LatentMapping, k_num_latents> k_mappings{{
    {Role::Lead, Feature::Openness, -2.5F, 2.0F, -3.0F, "dynamics"},
    {Role::Lead, Feature::Pinch, 0.5F, -3.0F, 0.0F, "body"},
    {Role::Lead, Feature::Y, -2.5F, 3.0F, 0.0F, "register"},
    {Role::Lead, Feature::X, -2.5F, 2.5F, 0.0F, "colour"},
    {Role::Texture, Feature::Y, -2.5F, 3.0F, 0.0F, "swell"},
    {Role::Lead, Feature::Roll, -3.0F, 3.0F, 0.0F, "tilt"},
    {Role::Texture, Feature::X, -3.0F, 3.0F, 0.0F, "dark/bright"},
    {Role::Texture, Feature::Openness, 0.0F, 3.0F, 0.0F, "resonance"},
}};
// ------------------------------------------------------------------------------

float normalize(float value, float at_0, float at_1) {
    return std::clamp((value - at_0) / (at_1 - at_0), 0.0F, 1.0F);
}

// Mirrored (selfie) and aspect-corrected: x grows to the right of the screen, y downwards.
ht::Vec2 screen(const ht::Hand& hand, int index, float aspect) {
    const auto& p = hand.landmarks[static_cast<size_t>(index)];
    return {(1.0F - p.x) * aspect, p.y};
}

float distance(ht::Vec2 a, ht::Vec2 b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

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

HandFeatures compute_features(const ht::Hand& hand, float aspect) {
    const ht::Vec2 wrist = screen(hand, k_wrist, aspect);
    const ht::Vec2 middle_mcp = screen(hand, k_middle_mcp, aspect);
    const float palm = std::max(distance(wrist, middle_mcp), 1e-4F);

    ht::Vec2 centre{};
    for (int index : {k_wrist, k_index_mcp, k_middle_mcp, k_ring_mcp, k_pinky_mcp}) {
        const auto& p = hand.landmarks[static_cast<size_t>(index)];
        centre.x += (1.0F - p.x) / 5.0F;
        centre.y += p.y / 5.0F;
    }

    float tip_distance = 0.0F;
    for (int tip : {k_index_tip, k_middle_tip, k_ring_tip, k_pinky_tip})
        tip_distance += distance(screen(hand, tip, aspect), wrist) / 4.0F;

    const float pinch_distance = distance(screen(hand, k_thumb_tip, aspect), screen(hand, k_index_tip, aspect));
    const float roll = std::atan2(middle_mcp.x - wrist.x, wrist.y - middle_mcp.y);  // 0 = fingers up

    HandFeatures features;
    features.x = normalize(centre.x, k_edge_margin, 1.0F - k_edge_margin);
    features.y = normalize(1.0F - centre.y, k_edge_margin, 1.0F - k_edge_margin);
    features.openness = normalize(tip_distance / palm, k_fist_ratio, k_open_ratio);
    features.pinch = normalize(pinch_distance / palm, k_pinch_open_ratio, k_pinch_closed_ratio);
    features.roll = normalize(roll, -k_roll_range, k_roll_range);
    return features;
}

std::array<const ht::Hand*, k_num_roles> assign_roles(const ht::Result& result, bool swap) {
    std::array<const ht::Hand*, k_num_roles> roles{nullptr, nullptr};
    if (!result.valid || result.num_hands == 0)
        return roles;

    // The two most confident hands.
    const ht::Hand* first = nullptr;
    const ht::Hand* second = nullptr;
    for (size_t i = 0; i < result.num_hands; ++i) {
        const ht::Hand* hand = &result.hands[i];
        if (first == nullptr || hand->presence > first->presence) {
            second = first;
            first = hand;
        } else if (second == nullptr || hand->presence > second->presence) {
            second = hand;
        }
    }

    auto& lead = roles[static_cast<size_t>(Role::Lead)];
    auto& texture = roles[static_cast<size_t>(Role::Texture)];
    if (second == nullptr) {
        lead = first;
        return roles;
    }

    const auto leads = [swap](const ht::Hand* hand) { return (hand->handedness_right > 0.5F) != swap; };
    if (leads(first) != leads(second)) {
        lead = leads(first) ? first : second;
        texture = leads(first) ? second : first;
    } else {
        // Same handedness on both: the hand further right on screen (mirrored) leads, unless swapped.
        const bool first_right = first->landmarks[k_wrist].x < second->landmarks[k_wrist].x;
        lead = first_right != swap ? first : second;
        texture = lead == first ? second : first;
    }
    return roles;
}

Latents HandControls::rest_latents() {
    Latents rest{};
    for (size_t i = 0; i < rest.size(); ++i)
        rest[i] = mappings()[i].rest;
    return rest;
}

void HandControls::update(const ht::Result& result, float aspect, double dt_seconds, const Settings& settings) {
    const auto hands = assign_roles(result, settings.swap_hands);
    std::array<std::optional<HandFeatures>, k_num_roles> features;
    for (size_t role = 0; role < k_num_roles; ++role) {
        if (hands[role] != nullptr) { features[role] = compute_features(*hands[role], aspect); }
        m_slots[role] = hands[role] != nullptr ? hands[role]->slot : -1;
    }
    updateFeatures(features[0], features[1], dt_seconds, settings);
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
