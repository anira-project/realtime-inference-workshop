#pragma once

#include <anira/anira.h>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "Types.h"

// Public interface: the MediaPipe gesture-recognition chain running on anira.
namespace anira::models::handtracking {

inline constexpr int k_max_hands = 4;

inline constexpr std::array<const char*, 8> k_gesture_labels{
    "None",       "Closed_Fist", "Open_Palm", "Pointing_Up",
    "Thumb_Down", "Thumb_Up",    "Victory",   "ILoveYou"};

// MediaPipe hand skeleton edges (landmark index pairs).
inline constexpr int k_num_hand_connections = 21;
inline constexpr int k_hand_connections[k_num_hand_connections][2] = {
    {0, 1},   {1, 2},   {2, 3},   {3, 4},    // thumb
    {0, 5},   {5, 6},   {6, 7},   {7, 8},    // index
    {5, 9},   {9, 10},  {10, 11}, {11, 12},  // middle
    {9, 13},  {13, 14}, {14, 15}, {15, 16},  // ring
    {13, 17}, {17, 18}, {18, 19}, {19, 20},  // pinky
    {0, 17}};                                // palm

// Backends this binary was built with.
struct BackendChoice {
    const char* name;
    anira::InferenceBackend backend;
};

inline std::vector<BackendChoice> available_backends() {
    std::vector<BackendChoice> backends;
#ifdef USE_LITERT
    backends.push_back({"LiteRT", anira::InferenceBackend::LITERT});
#endif
#ifdef USE_TFLITE
    backends.push_back({"TFLite", anira::InferenceBackend::TFLITE});
#endif
#ifdef USE_ONNXRUNTIME
    backends.push_back({"ONNX Runtime", anira::InferenceBackend::ONNX});
#endif
#ifdef USE_LIBTORCH
    backends.push_back({"LibTorch", anira::InferenceBackend::LIBTORCH});
#endif
    return backends;
}

inline const char* backend_name(anira::InferenceBackend backend) {
    for (const auto& choice : available_backends())
        if (choice.backend == backend)
            return choice.name;
    return "unknown";
}

// Runtime-adjustable settings, mirroring MediaPipe's GestureRecognizerOptions.
struct Options {
    int num_hands = 2;  // 1 .. k_max_hands
    float min_hand_detection_confidence = 0.5F;  // palm detector score
    float min_hand_presence_confidence = 0.5F;   // landmark presence score
    float min_tracking_confidence = 0.5F;  // IoU for detection/track association
};

struct Hand {
    int slot = 0;           // stable slot index (also used for overlay colors)
    bool tracking = false;  // ROI came from previous landmarks (no detector)
    float presence = 0.0F;
    float handedness_right = 0.0F;  // probability the hand is a right hand
    int gesture = -1;               // index into k_gesture_labels, -1 = none
    float gesture_score = 0.0F;
    std::array<Vec2, 21> landmarks;   // normalized full-frame coords
    std::array<Vec2, 4> roi_corners;  // normalized full-frame coords
};

// Trivially copyable — no frame reference; the host draws the normalized overlay itself.
struct Result {
    bool valid = false;  // tracker produced a result for a frame
    std::array<Hand, k_max_hands> hands;
    std::size_t num_hands = 0;
    double callback_ms = 0.0;   // Streamed: glue time only; Synchronous: full chain
    double camera_fps = 0.0;    // rate of frames entering the tracker
    double tracking_fps = 0.0;  // rate of frames that completed hand results
    anira::InferenceBackend backend = anira::InferenceBackend::CUSTOM;
};

class HandTracker {
public:
    virtual ~HandTracker() = default;

    // Called per camera frame on the capture thread; never blocks on inference.
    virtual void processFrame(const Frame& frame) = 0;

    // Newest published result. Single reader: call from one (GUI) thread.
    virtual Result getLatestResult() = 0;

    // Thread-safe; callable from any thread.
    virtual void setOptions(const Options& options) = 0;
    virtual Options getOptions() const = 0;

    // Applied asynchronously between frames (in-flight work drained, tracking resets).
    virtual void setBackend(anira::InferenceBackend backend) = 0;
    virtual anira::InferenceBackend getBackend() const = 0;
};

enum class Engine { Streamed, Synchronous };

inline const char* engine_name(Engine engine) {
    return engine == Engine::Streamed ? "Streamed" : "Synchronous";
}

// Expensive (loads models); call off the realtime path.
std::unique_ptr<HandTracker> createHandTracker(Engine engine);

}  // namespace anira::models::handtracking
