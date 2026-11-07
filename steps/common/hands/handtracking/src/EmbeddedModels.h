#pragma once

#include <cstddef>
#include <span>

// The models, compiled in by JUCE's binary data target (steps/common/hands/hands.cmake):
// one const char* + <name>Size pair per file.
#include <BinaryData.h>

namespace anira::models::handtracking::embedded {

// The bytes of one compiled-in model; static, so anira may borrow them.
inline std::span<const std::byte> bytes_of(const char* data, int size) {
    return {reinterpret_cast<const std::byte*>(data), static_cast<std::size_t>(size)};
}

inline std::span<const std::byte> hand_detector() {
    return bytes_of(BinaryData::hand_detector_onnx, BinaryData::hand_detector_onnxSize);
}
inline std::span<const std::byte> hand_landmarks_detector() {
    return bytes_of(BinaryData::hand_landmarks_detector_onnx,
                    BinaryData::hand_landmarks_detector_onnxSize);
}
inline std::span<const std::byte> gesture_embedder() {
    return bytes_of(BinaryData::gesture_embedder_onnx, BinaryData::gesture_embedder_onnxSize);
}
inline std::span<const std::byte> canned_gesture_classifier() {
    return bytes_of(BinaryData::canned_gesture_classifier_onnx,
                    BinaryData::canned_gesture_classifier_onnxSize);
}

}  // namespace anira::models::handtracking::embedded
