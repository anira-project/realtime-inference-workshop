#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <utility>

#include <handtracking/HandTracker.h>

#include "Geometry.h"
#include "InferenceConfigs.h"
#include "MediaPipeOps.h"

namespace anira::models::handtracking {

inline constexpr float k_nms_iou_threshold = 0.3F;
inline constexpr float k_min_gesture_confidence = 0.5F;

inline double nowMs() {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Rotated square region of interest, in frame pixel coordinates.
struct RegionOfInterest {
    float center_x = 0.0F;
    float center_y = 0.0F;
    float size = 0.0F;      // side length of the square
    float rotation = 0.0F;  // radians, so that the hand points up in the crop
    Rect boundingBox() const {
        return {center_x - (size * 0.5F), center_y - (size * 0.5F), size, size};
    }
};

// EMA frame rate; decayIfStale() zeroes it once events stop arriving.
struct FpsMeter {
    double last_ms = 0.0;
    double fps = 0.0;

    void tick(double now_ms) {
        if (last_ms > 0.0 && now_ms > last_ms) {
            const double instant_fps = 1000.0 / (now_ms - last_ms);
            fps = (fps == 0.0) ? instant_fps : (0.9 * fps) + (0.1 * instant_fps);
        }
        last_ms = now_ms;
    }

    void decayIfStale(double now_ms, double timeout_ms = 1000.0) {
        if (now_ms - last_ms > timeout_ms)
            fps = 0.0;
    }
};

// Plain atomics: GUI thread writes, processing thread reads/applies.
class SharedTrackerState {
public:
    void setOptions(const Options& options) {
        m_num_hands.store(std::clamp(options.num_hands, 1, k_max_hands),
                          std::memory_order_relaxed);
        m_min_detection_confidence.store(options.min_hand_detection_confidence,
                                         std::memory_order_relaxed);
        m_min_presence_confidence.store(options.min_hand_presence_confidence,
                                        std::memory_order_relaxed);
        m_min_tracking_confidence.store(options.min_tracking_confidence,
                                        std::memory_order_relaxed);
    }

    Options getOptions() const {
        Options options;
        options.num_hands = m_num_hands.load(std::memory_order_relaxed);
        options.min_hand_detection_confidence =
            m_min_detection_confidence.load(std::memory_order_relaxed);
        options.min_hand_presence_confidence =
            m_min_presence_confidence.load(std::memory_order_relaxed);
        options.min_tracking_confidence =
            m_min_tracking_confidence.load(std::memory_order_relaxed);
        return options;
    }

    void requestBackend(anira::InferenceBackend backend) {
        m_requested_backend.store(static_cast<int>(backend), std::memory_order_relaxed);
    }

    anira::InferenceBackend requestedBackend() const {
        return static_cast<anira::InferenceBackend>(
            m_requested_backend.load(std::memory_order_relaxed));
    }

private:
    std::atomic<int> m_requested_backend{0};
    std::atomic<int> m_num_hands{2};
    std::atomic<float> m_min_detection_confidence{0.5F};
    std::atomic<float> m_min_presence_confidence{0.5F};
    std::atomic<float> m_min_tracking_confidence{0.5F};
};

// Captured at sampling time; needed to map detections back to frame pixels.
struct LetterboxGeometry {
    float side = 0.0F;  // side length of the square the frame was fit into
    float pad_x = 0.0F;
    float pad_y = 0.0F;
};

inline LetterboxGeometry letterboxGeometry(const Frame& frame) {
    const auto frame_width = static_cast<float>(frame.width);
    const auto frame_height = static_cast<float>(frame.height);
    const float side = std::fmax(frame_width, frame_height);
    return {side, (side - frame_width) * 0.5F, (side - frame_height) * 0.5F};
}

// dest -> source transform letterboxing the full frame into the square detector input.
inline Affine2D letterboxDestToSource(const LetterboxGeometry& geometry) {
    return Affine2D::translation(geometry.pad_x, geometry.pad_y)
        .scaled(static_cast<float>(k_detector_input_size) / geometry.side)
        .inverted();
}

// Threshold, sort, greedy NMS, then MediaPipe palm_detection_detection_to_roi (rotate wrist->middle up, scale 2.6, shift -0.5 along rotated y).
inline size_t decodePalmROIs(const float* boxes,     // [k_num_anchors, 18]
                             const float* scores,    // [k_num_anchors]
                             const std::vector<Vec2>& anchors,
                             float min_detection_confidence,
                             int max_hands,
                             const LetterboxGeometry& letterbox,
                             std::array<RegionOfInterest, k_max_hands>& rois) {
    std::array<std::pair<float, int>, k_num_anchors> candidates;
    size_t num_candidates = 0;
    for (int i = 0; i < k_num_anchors; ++i) {
        const float score =
            sigmoid(std::clamp(scores[static_cast<size_t>(i)], -100.0F, 100.0F));
        if (score >= min_detection_confidence)
            candidates[num_candidates++] = {score, i};
    }
    std::sort(candidates.begin(), candidates.begin() + static_cast<long>(num_candidates),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    // Palm box in normalized letterbox-square coords.
    const float inv_scale = 1.0F / static_cast<float>(k_detector_input_size);
    const auto decode_box = [&](int index) {
        const float* detection = &boxes[static_cast<size_t>(index) * k_num_detection_values];
        const auto anchor = anchors[static_cast<size_t>(index)];
        const float center_x = (detection[0] * inv_scale) + anchor.x;
        const float center_y = (detection[1] * inv_scale) + anchor.y;
        const float width = detection[2] * inv_scale;
        const float height = detection[3] * inv_scale;
        return Rect{center_x - (width * 0.5F), center_y - (height * 0.5F), width, height};
    };

    const auto to_frame_px = [&](float norm_x, float norm_y) {
        return Vec2{(norm_x * letterbox.side) - letterbox.pad_x,
                    (norm_y * letterbox.side) - letterbox.pad_y};
    };

    std::array<Rect, k_max_hands> kept_boxes;
    size_t num_kept = 0;

    for (size_t c = 0;
         c < num_candidates && num_kept < static_cast<size_t>(max_hands); ++c) {
        const int index = candidates[c].second;
        const auto box = decode_box(index);

        bool suppressed = false;
        for (size_t k = 0; k < num_kept; ++k) {
            if (intersectionOverUnion(box, kept_boxes[k]) > k_nms_iou_threshold) {
                suppressed = true;
                break;
            }
        }
        if (suppressed)
            continue;

        const float* detection = &boxes[static_cast<size_t>(index) * k_num_detection_values];
        const auto anchor = anchors[static_cast<size_t>(index)];

        const auto box_center = to_frame_px(box.centreX(), box.centreY());
        const float box_width = box.width * letterbox.side;
        const float box_height = box.height * letterbox.side;

        // Keypoint 0 = wrist center, keypoint 2 = middle finger MCP.
        const auto wrist = to_frame_px((detection[4] * inv_scale) + anchor.x,
                                       (detection[5] * inv_scale) + anchor.y);
        const auto middle_mcp = to_frame_px((detection[8] * inv_scale) + anchor.x,
                                            (detection[9] * inv_scale) + anchor.y);

        const float rotation = rotationFromPoints(wrist, middle_mcp);
        const auto roi_center = shiftAlongRotatedY(box_center, rotation, -0.5F, box_height);

        kept_boxes[num_kept] = box;
        rois[num_kept++] = {roi_center.x, roi_center.y,
                           std::fmax(box_width, box_height) * 2.6F, rotation};
    }
    return num_kept;
}

// IoU alone is unreliable (detection 2.6x vs tracking 2.0x ROI sizes differ) — also accept center-inside, or the same hand occupies a second slot.
inline bool roiMatchesTracked(const RegionOfInterest& roi,
                              const RegionOfInterest& tracked,
                              float min_tracking_confidence) {
    const auto tracked_box = tracked.boundingBox();
    return intersectionOverUnion(roi.boundingBox(), tracked_box) > min_tracking_confidence ||
           tracked_box.contains({roi.center_x, roi.center_y});
}

// frame -> crop transform for a hand ROI (the landmark model's input view).
inline Affine2D roiFrameToCrop(const RegionOfInterest& roi) {
    const auto input_side = static_cast<float>(k_landmark_input_size);
    return Affine2D::translation(-roi.center_x, -roi.center_y)
        .rotated(-roi.rotation)
        .scaled(input_side / roi.size)
        .translated(input_side * 0.5F, input_side * 0.5F);
}

// MediaPipe hand_landmark_landmarks_to_roi: rotate wrist->middle-MCP up, bbox long side * 2.0, shift -0.1.
inline RegionOfInterest roiFromLandmarks(const std::array<Vec2, 21>& landmarks_px,
                                         float& rotation_out) {
    auto min_x = landmarks_px[0].x, max_x = landmarks_px[0].x;
    auto min_y = landmarks_px[0].y, max_y = landmarks_px[0].y;
    for (const auto& point : landmarks_px) {
        min_x = std::fmin(min_x, point.x);
        max_x = std::fmax(max_x, point.x);
        min_y = std::fmin(min_y, point.y);
        max_y = std::fmax(max_y, point.y);
    }

    const float rotation = rotationFromPoints(landmarks_px[0], landmarks_px[9]);
    const float roi_size = std::fmax(max_x - min_x, max_y - min_y) * 2.0F;
    const auto roi_center = shiftAlongRotatedY(
        {(min_x + max_x) * 0.5F, (min_y + max_y) * 0.5F}, rotation, -0.1F, roi_size);

    rotation_out = rotation;
    return {roi_center.x, roi_center.y, roi_size, rotation};
}

// {label index, score}; -1 when best is "None" or below threshold. Manual softmax fallback if outputs don't look like probabilities.
inline std::pair<int, float> classifyGestureScores(std::array<float, 8>& scores) {
    float sum = 0.0F;
    for (const float score : scores)
        sum += score;
    if (sum < 0.99F || sum > 1.01F) {
        const float max_score = *std::max_element(scores.begin(), scores.end());
        sum = 0.0F;
        for (auto& score : scores) {
            score = std::exp(score - max_score);
            sum += score;
        }
        for (auto& score : scores)
            score /= sum;
    }

    const auto best = std::max_element(scores.begin(), scores.end());
    const auto best_index = static_cast<int>(best - scores.begin());
    if (best_index == 0 || *best < k_min_gesture_confidence)  // 0 = "None"
        return {-1, *best};
    return {best_index, *best};
}

}  // namespace anira::models::handtracking
