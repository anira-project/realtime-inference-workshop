#include "HandControls.h"

#include <cmath>
#include <utility>

namespace hands {

namespace {

// Spread widens (> 1) or narrows (< 1) a feature around its middle, 0.5.
HandFeatures spread(HandFeatures features, float amount) {
    for (float* feature : {&features.x, &features.y, &features.openness, &features.pinch, &features.roll})
        *feature = 0.5F + (*feature - 0.5F) * amount;
    return features;
}

}  // namespace

void HandControls::update(const std::vector<Hand>& hands, double dt_seconds, const Settings& settings) {
    // Up to two hands, the most confident first: the hand with is_right !=
    // swap_hands leads, the other one colours, a single hand always leads.
    const Hand* lead = nullptr;
    const Hand* texture = nullptr;
    for (const Hand& hand : hands) {
        if (hand.is_right != settings.swap_hands && lead == nullptr)
            lead = &hand;
        else if (texture == nullptr)
            texture = &hand;
    }
    if (lead == nullptr)
        std::swap(lead, texture);

    m_hand_ids = {lead != nullptr ? lead->id : -1, texture != nullptr ? texture->id : -1};
    updateFeatures(lead != nullptr ? std::optional(lead->features) : std::nullopt,
                   texture != nullptr ? std::optional(texture->features) : std::nullopt,
                   dt_seconds, settings);
}

void HandControls::updateFeatures(const std::optional<HandFeatures>& lead,
                                  const std::optional<HandFeatures>& texture,
                                  double dt_seconds,
                                  const Settings& settings) {
    // A hand that drops out for less than k_hold_seconds keeps playing
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

    // Your mapping while the lead hand is there; rest without it
    const auto& lead_features = m_features[static_cast<size_t>(Role::Lead)];
    const auto& texture_features = m_features[static_cast<size_t>(Role::Texture)];
    if (lead_features.has_value()) {
        m_targets = map_hands(spread(*lead_features, settings.spread),
                              texture_features.has_value() ? std::optional(spread(*texture_features, settings.spread))
                                                           : std::nullopt);
    } else {
        m_targets = k_rest;
    }

    // One pole per latent: quick while a hand plays, a slow glide to rest
    const double tau_ms = lead_features.has_value() ? settings.smoothing_ms : k_release_ms;
    const auto coefficient = tau_ms <= 0.0 ? 1.0F : static_cast<float>(1.0 - std::exp(-dt_seconds * 1000.0 / tau_ms));
    for (size_t i = 0; i < k_num_latents; ++i)
        m_latents[i] += (m_targets[i] - m_latents[i]) * coefficient;
}

}  // namespace hands
