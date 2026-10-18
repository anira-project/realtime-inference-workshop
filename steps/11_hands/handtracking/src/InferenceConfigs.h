#pragma once

#include <anira/anira.h>
#include <thread>
#include <vector>

#include <handtracking/HandTracker.h>
#include <handtracking/Types.h>

#include "EmbeddedModels.h"

// anira configs + model constants (MediaPipe gesture_recognizer.task; image inputs are NHWC RGB in [0, 1]).
namespace anira::models::handtracking {

// anira requires an identical wait strategy across all CoreConfigs in a process — construct every InferenceHandler with this config.
inline anira::CoreConfig make_context_config() {
    return anira::CoreConfig(std::thread::hardware_concurrency(),
                                anira::WaitStrategy::Blocking);
}

// Worst-case frame rate into a tracker; anira sizes its ring buffers from it.
inline constexpr double k_camera_frame_rate = 30.0;

// All models are stateless, so sessions share pooled processors; a stateful model would need an exclusive processor to keep inferences ordered.
inline constexpr bool k_session_exclusive_processor = false;

// Per-backend max_inference_time deadlines (Streamed engine): 2x the benchmarked worst case (M3 Max); determine latency and queue sizing.
inline float detector_deadline_ms(anira::InferenceBackend backend) {
    if (backend == anira::InferenceBackend::ONNX)
        return 76.0F;  // worst 38.0 ms
#ifdef USE_LIBTORCH
    if (backend == anira::InferenceBackend::LIBTORCH)
        return 193.0F;  // worst 96.5 ms
#endif
    return 23.4F;       // LiteRT, worst 11.7 ms
}

inline float landmark_deadline_ms(anira::InferenceBackend backend) {
    if (backend == anira::InferenceBackend::ONNX)
        return 39.2F;  // worst 19.6 ms
#ifdef USE_LIBTORCH
    if (backend == anira::InferenceBackend::LIBTORCH)
        return 114.4F;  // worst 57.2 ms
#endif
    return 21.0F;       // LiteRT, worst 10.5 ms
}

// Must cover thread-pool queueing behind detector/landmark jobs, not just the ~0.1 ms execution: too small -> too few in-flight slots, dropped pushes, desynced session.
inline constexpr float k_gesture_stage_deadline_ms = 200.0F;

// Non-realtime blocking mode: the deadline only sizes buffers and can never be missed.
inline constexpr float k_sync_deadline_ms = 200.0F;

inline constexpr int k_detector_input_size = 192;   // 192x192x3
inline constexpr int k_num_anchors = 2016;
inline constexpr int k_num_detection_values = 18;   // 4 box + 7 keypoints x 2

inline anira::InferenceConfig make_detector_config() {
    return anira::InferenceConfig(
        embedded::hand_detector_models(),
        {anira::TensorShape({{1, 192, 192, 3}},           // NHWC image input
                            {{1, 2016, 18}, {1, 2016, 1}})},  // raw boxes+keypoints, raw scores
        anira::ProcessingSpec({1}, {1, 1}, {192 * 192 * 3}, {2016 * 18, 2016}),
        detector_deadline_ms(anira::InferenceBackend::ONNX),  // overwritten per backend/engine
        1,  // one warm-up inference
        k_session_exclusive_processor);
}

// SSD anchors: strides {8, 16, 16, 16}, 2 anchors/cell on the 24x24 map, 6 on the 12x12 map -> 2016.
inline std::vector<Vec2> generate_ssd_anchors() {
    constexpr int strides[4]{8, 16, 16, 16};
    std::vector<Vec2> anchors;

    size_t layer_id = 0;
    while (layer_id < 4) {
        size_t last_same_stride_layer = layer_id;
        int anchors_per_cell = 0;
        while (last_same_stride_layer < 4 &&
               strides[last_same_stride_layer] == strides[layer_id]) {
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

inline anira::InferenceConfig make_landmark_config() {
    return anira::InferenceConfig(
        embedded::hand_landmarks_detector_models(),
        {anira::TensorShape({{1, 224, 224, 3}},                    // NHWC image input
                            {{1, 63}, {1, 1}, {1, 1}, {1, 63}})},  // landmarks, presence,
                                                                   // handedness, world landmarks
        // Scalar presence/handedness outputs are non-streamable (size 0): read via PrePostProcessor::get_output.
        anira::ProcessingSpec({1}, {1, 1, 1, 1}, {224 * 224 * 3}, {63, 0, 0, 63}),
        landmark_deadline_ms(anira::InferenceBackend::ONNX),  // overwritten per backend/engine
        1,
        k_session_exclusive_processor);
}

inline anira::InferenceConfig make_embedder_config() {
    return anira::InferenceConfig(
        embedded::gesture_embedder_models(),
        {anira::TensorShape({{1, 21, 3}, {1, 1}, {1, 21, 3}},  // hand, handedness, world_hand
                            {{1, 128}})},
        // Handedness stays streamable although scalar: non-streamable tensors have latest-value semantics and concurrent hands would race on it.
        anira::ProcessingSpec({1, 1, 1}, {1}, {63, 1, 63}, {128}),
        k_gesture_stage_deadline_ms,
        1,
        k_session_exclusive_processor);
}

inline anira::InferenceConfig make_classifier_config() {
    return anira::InferenceConfig(
        embedded::canned_gesture_classifier_models(),
        {anira::TensorShape({{1, 128}}, {{1, 8}})},
        anira::ProcessingSpec({1}, {1}, {128}, {8}),
        k_gesture_stage_deadline_ms,
        1,
        k_session_exclusive_processor);
}

}  // namespace anira::models::handtracking
