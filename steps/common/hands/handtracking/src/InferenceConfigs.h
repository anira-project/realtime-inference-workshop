#pragma once

#include <handtracking/HandTracker.h>
#include <handtracking/Types.h>

#include <anira/anira.hpp>
#include <span>
#include <string_view>
#include <vector>

#include "EmbeddedModels.h"

// The models as anira model configs, and their constants (MediaPipe gesture_recognizer.task;
// image inputs are NHWC RGB in [0, 1]). Every input and output is Streamed with one window per
// inference: a camera frame or a hand is pushed whole and its result popped whole.
namespace anira::models::handtracking {

// A model document with the compiled-in ONNX bytes in place of its path. The bytes are static,
// so anira borrows them.
inline anira::ModelConfig make_model(std::string_view document, std::span<const std::byte> bytes) {
    anira::ModelConfig model = anira::ModelConfig::from_json(document);
    model.set_model_bytes(0, bytes, ANIRA_BYTES_BORROW);
    return model;
}

// Worst-case frame rate into a tracker; anira sizes its ring buffers from it.
inline constexpr double k_camera_frame_rate = 30.0;

// Per-model budgets: 2x the benchmarked worst case on ONNX Runtime (M3 Max); they set the
// latency and the queue sizes.
inline constexpr float k_detector_deadline_ms = 76.0F;  // worst 38.0 ms
inline constexpr float k_landmark_deadline_ms = 39.2F;  // worst 19.6 ms
// Must cover thread-pool queueing behind detector/landmark jobs, not just the ~0.1 ms execution:
// too small -> too few in-flight slots, dropped pushes, desynced session.
inline constexpr float k_gesture_stage_deadline_ms = 200.0F;

inline constexpr int k_detector_input_size = 192;  // 192x192x3
inline constexpr int k_num_anchors = 2016;
inline constexpr int k_num_detection_values = 18;  // 4 box + 7 keypoints x 2

// In: the letterboxed frame. Out: raw boxes and keypoints, raw scores.
inline constexpr std::string_view k_detector_document = R"({
  "models": [{ "engine": "onnxruntime", "path": "anira:bytes" }],
  "state": "stateless",
  "anchor": "input_0",
  "inputs": [
    { "name": "input_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["any", 192], ["any", 192], ["time", 3]],
      "window": { "min": 110592, "max": 110592 }, "overlap": 0 }
  ],
  "outputs": [
    { "name": "output_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["any", 2016], ["time", 18]],
      "window": { "min": 36288, "max": 36288 }, "overlap": 0 },
    { "name": "output_1", "dtype": "float32", "role": "streamed",
      "axes": [["any", 1], ["time", 2016], ["channel", 1]],
      "window": { "min": 2016, "max": 2016 }, "overlap": 0 }
  ]
})";

inline anira::ModelConfig make_detector_model() {
    return make_model(k_detector_document, embedded::hand_detector());
}

// SSD anchors: strides {8, 16, 16, 16}, 2 anchors/cell on the 24x24 map, 6 on the 12x12 map ->
// 2016.
inline std::vector<Vec2> generate_ssd_anchors() {
    constexpr int strides[4]{8, 16, 16, 16};
    std::vector<Vec2> anchors;

    size_t layer_id = 0;
    while (layer_id < 4) {
        size_t last_same_stride_layer = layer_id;
        int anchors_per_cell = 0;
        while (last_same_stride_layer < 4 && strides[last_same_stride_layer] == strides[layer_id]) {
            ++last_same_stride_layer;
            anchors_per_cell += 2;  // aspect ratio 1.0 + interpolated scale
        }

        const int feature_map_size = k_detector_input_size / strides[layer_id];
        for (int y = 0; y < feature_map_size; ++y) {
            for (int x = 0; x < feature_map_size; ++x) {
                for (int anchor = 0; anchor < anchors_per_cell; ++anchor) {
                    anchors.push_back(
                        {(static_cast<float>(x) + 0.5F) / static_cast<float>(feature_map_size),
                         (static_cast<float>(y) + 0.5F) / static_cast<float>(feature_map_size)});
                }
            }
        }
        layer_id = last_same_stride_layer;
    }
    return anchors;
}

inline constexpr int k_landmark_input_size = 224;  // 224x224x3
inline constexpr int k_num_landmarks = 21;

// In: the hand crop. Out: landmarks, presence, handedness, world landmarks. Presence and
// handedness are Static: popped together with the landmarks, each the latest value anira has.
inline constexpr std::string_view k_landmark_document = R"({
  "models": [{ "engine": "onnxruntime", "path": "anira:bytes" }],
  "state": "stateless",
  "anchor": "input_0",
  "inputs": [
    { "name": "input_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["any", 224], ["any", 224], ["time", 3]],
      "window": { "min": 150528, "max": 150528 }, "overlap": 0 }
  ],
  "outputs": [
    { "name": "output_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["time", 63]],
      "window": { "min": 63, "max": 63 }, "overlap": 0 },
    { "name": "output_1", "dtype": "float32", "role": "static", "axes": [["any", 1], ["any", 1]] },
    { "name": "output_2", "dtype": "float32", "role": "static", "axes": [["any", 1], ["any", 1]] },
    { "name": "output_3", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["time", 63]],
      "window": { "min": 63, "max": 63 }, "overlap": 0 }
  ]
})";

inline anira::ModelConfig make_landmark_model() {
    return make_model(k_landmark_document, embedded::hand_landmarks_detector());
}

// In: hand, handedness, world hand. Handedness stays Streamed although scalar: a Static value is
// the latest one, and concurrent hands would race on it. Out: the embedding.
inline constexpr std::string_view k_embedder_document = R"({
  "models": [{ "engine": "onnxruntime", "path": "anira:bytes" }],
  "state": "stateless",
  "anchor": "input_0",
  "inputs": [
    { "name": "input_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["any", 21], ["time", 3]],
      "window": { "min": 63, "max": 63 }, "overlap": 0 },
    { "name": "input_1", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["time", 1]],
      "window": { "min": 1, "max": 1 }, "overlap": 0 },
    { "name": "input_2", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["any", 21], ["time", 3]],
      "window": { "min": 63, "max": 63 }, "overlap": 0 }
  ],
  "outputs": [
    { "name": "output_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["time", 128]],
      "window": { "min": 128, "max": 128 }, "overlap": 0 }
  ]
})";

inline anira::ModelConfig make_embedder_model() {
    return make_model(k_embedder_document, embedded::gesture_embedder());
}

// In: the embedding. Out: the eight gesture scores.
inline constexpr std::string_view k_classifier_document = R"({
  "models": [{ "engine": "onnxruntime", "path": "anira:bytes" }],
  "state": "stateless",
  "anchor": "input_0",
  "inputs": [
    { "name": "input_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["time", 128]],
      "window": { "min": 128, "max": 128 }, "overlap": 0 }
  ],
  "outputs": [
    { "name": "output_0", "dtype": "float32", "role": "streamed",
      "axes": [["channel", 1], ["time", 8]],
      "window": { "min": 8, "max": 8 }, "overlap": 0 }
  ]
})";

inline anira::ModelConfig make_classifier_model() {
    return make_model(k_classifier_document, embedded::canned_gesture_classifier());
}

}  // namespace anira::models::handtracking
