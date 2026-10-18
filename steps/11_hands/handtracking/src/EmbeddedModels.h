#pragma once

#include <anira/anira.h>
#include <cstddef>
#include <vector>

// The models, compiled in by JUCE's binary data target (steps/11_hands/CMakeLists.txt):
// one const char* + <name>Size pair per file, which is the naming this header expects.
#include <BinaryData.h>

// The USE_* guards must mirror what CMake passes to the generator, so a referenced symbol always has a definition.
namespace anira::models::handtracking::embedded {

using BinaryData::hand_detector_onnx;
using BinaryData::hand_detector_onnxSize;
using BinaryData::hand_landmarks_detector_onnx;
using BinaryData::hand_landmarks_detector_onnxSize;
using BinaryData::gesture_embedder_onnx;
using BinaryData::gesture_embedder_onnxSize;
using BinaryData::canned_gesture_classifier_onnx;
using BinaryData::canned_gesture_classifier_onnxSize;

// const_cast is safe — binary ModelData only reads through the pointer.
#if defined(USE_LITERT)
#define HT_ADD_LITERT(models, base)                                            \
    (models).emplace_back(const_cast<char*>(base##_tflite),                    \
                          static_cast<size_t>(base##_tfliteSize),              \
                          anira::InferenceBackend::LITERT);
#else
#define HT_ADD_LITERT(models, base)
#endif

#if defined(USE_TFLITE)
#define HT_ADD_TFLITE(models, base)                                            \
    (models).emplace_back(const_cast<char*>(base##_tflite),                    \
                          static_cast<size_t>(base##_tfliteSize),              \
                          anira::InferenceBackend::TFLITE);
#else
#define HT_ADD_TFLITE(models, base)
#endif

#if defined(USE_ONNXRUNTIME)
#define HT_ADD_ONNX(models, base)                                              \
    (models).emplace_back(const_cast<char*>(base##_onnx),                      \
                          static_cast<size_t>(base##_onnxSize),                \
                          anira::InferenceBackend::ONNX);
#else
#define HT_ADD_ONNX(models, base)
#endif

#if defined(USE_LIBTORCH)
#define HT_ADD_LIBTORCH(models, base)                                          \
    (models).emplace_back(const_cast<char*>(base##_pt),                        \
                          static_cast<size_t>(base##_ptSize),                  \
                          anira::InferenceBackend::LIBTORCH);
#else
#define HT_ADD_LIBTORCH(models, base)
#endif

#define HT_EMBEDDED_MODEL_DATA_FN(fn_name, base)                               \
    inline std::vector<anira::ModelData> fn_name() {                           \
        std::vector<anira::ModelData> models;                                  \
        HT_ADD_LITERT(models, base)                                            \
        HT_ADD_TFLITE(models, base)                                            \
        HT_ADD_ONNX(models, base)                                              \
        HT_ADD_LIBTORCH(models, base)                                          \
        return models;                                                         \
    }

HT_EMBEDDED_MODEL_DATA_FN(hand_detector_models, hand_detector)
HT_EMBEDDED_MODEL_DATA_FN(hand_landmarks_detector_models, hand_landmarks_detector)
HT_EMBEDDED_MODEL_DATA_FN(gesture_embedder_models, gesture_embedder)
HT_EMBEDDED_MODEL_DATA_FN(canned_gesture_classifier_models, canned_gesture_classifier)

#undef HT_EMBEDDED_MODEL_DATA_FN
#undef HT_ADD_LITERT
#undef HT_ADD_TFLITE
#undef HT_ADD_ONNX
#undef HT_ADD_LIBTORCH

}  // namespace anira::models::handtracking::embedded
