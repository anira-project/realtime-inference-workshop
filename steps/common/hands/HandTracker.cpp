// The tracker behind HandTracker.h: anira-cam's MediaPipe chain (handtracking/)
// fed with JUCE images, and its 21 landmarks per hand turned into features.
#include "HandTracker.h"

#include <handtracking/HandTracker.h>

#include "ContextSettings.h"

#include <algorithm>
#include <atomic>
#include <cmath>

namespace hands {

namespace ht = anira::models::handtracking;

namespace {

constexpr int k_max_hands = 2;

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
constexpr float k_fist_ratio = 1.0F;          // mean fingertip-to-wrist distance, closed fist
constexpr float k_open_ratio = 1.75F;         // ... open palm
constexpr float k_pinch_closed_ratio = 0.2F;  // thumb tip to index tip, touching
constexpr float k_pinch_open_ratio = 0.8F;
constexpr float k_roll_range = 1.0F;    // radians each side of upright
constexpr float k_edge_margin = 0.15F;  // x/y reach the ends this far inside the frame

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

// `aspect` is frame width / height: the landmarks are normalized per axis.
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

Hand to_hand(const ht::Hand& tracked, float aspect) {
    Hand hand;
    hand.id = tracked.slot;
    hand.is_right = tracked.handedness_right > 0.5F;
    hand.confidence = tracked.presence;
    hand.features = compute_features(tracked, aspect);
    if (tracked.gesture > 0 && static_cast<size_t>(tracked.gesture) < ht::k_gesture_labels.size())
        hand.gesture = ht::k_gesture_labels[static_cast<size_t>(tracked.gesture)];
    for (size_t i = 0; i < hand.landmarks.size(); ++i)
        hand.landmarks[i] = {1.0F - tracked.landmarks[i].x, tracked.landmarks[i].y};
    return hand;
}

// The tracker may keep the frame beyond the call: keep_alive holds the image's refcount.
ht::Frame to_frame(const juce::Image& image) {
    auto retained = std::make_shared<juce::Image>(image);
    const juce::Image::BitmapData data(*retained, juce::Image::BitmapData::readOnly);
    ht::Frame frame;
    frame.pixels = data.data;
    frame.width = data.width;
    frame.height = data.height;
    frame.stride = data.lineStride;
    frame.pixel_stride = data.pixelStride;
    frame.format = ht::PixelFormat::BGRA;  // JUCE ARGB on little-endian
    frame.keep_alive = std::move(retained);
    return frame;
}

}  // namespace

struct HandTracker::Impl {
    std::unique_ptr<ht::HandTracker> tracker = ht::createHandTracker(context_config());
    std::atomic<float> aspect{16.0F / 9.0F};  // of the camera frames
    ht::Result result;                        // the last one hands() read
};

HandTracker::HandTracker() : m_impl(std::make_unique<Impl>()) {
    auto options = m_impl->tracker->getOptions();
    options.num_hands = k_max_hands;
    m_impl->tracker->setOptions(options);
}

HandTracker::~HandTracker() = default;

void HandTracker::pushFrame(const juce::Image& frame) {
    if (!frame.isValid())
        return;
    m_impl->aspect.store(static_cast<float>(frame.getWidth()) / static_cast<float>(frame.getHeight()),
                         std::memory_order_relaxed);
    m_impl->tracker->processFrame(to_frame(frame));
}

std::vector<Hand> HandTracker::hands() {
    m_impl->result = m_impl->tracker->getLatestResult();
    const auto& result = m_impl->result;

    std::vector<Hand> hands;
    if (!result.valid)
        return hands;
    const float aspect = m_impl->aspect.load(std::memory_order_relaxed);
    for (size_t i = 0; i < result.num_hands; ++i)
        hands.push_back(to_hand(result.hands[i], aspect));
    std::sort(hands.begin(), hands.end(), [](const Hand& a, const Hand& b) { return a.confidence > b.confidence; });
    if (hands.size() > static_cast<size_t>(k_max_hands))
        hands.resize(static_cast<size_t>(k_max_hands));
    return hands;
}

double HandTracker::cameraFps() const {
    return m_impl->result.camera_fps;
}

double HandTracker::trackingFps() const {
    return m_impl->result.tracking_fps;
}

juce::String HandTracker::engine() const {
    return ht::k_engine_name;
}

}  // namespace hands
