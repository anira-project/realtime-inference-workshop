#pragma once

#include <anira/anira.hpp>

#include <array>
#include <cstddef>
#include <memory>

#include "Types.h"

// Public interface: the MediaPipe gesture-recognition chain running on anira.
namespace anira::models::handtracking {

inline constexpr int k_max_hands = 4;

inline constexpr std::array<const char*, 8> k_gesture_labels{"None",
                                                             "Closed_Fist",
                                                             "Open_Palm",
                                                             "Pointing_Up",
                                                             "Thumb_Down",
                                                             "Thumb_Up",
                                                             "Victory",
                                                             "ILoveYou"};

// MediaPipe hand skeleton edges (landmark index pairs).
inline constexpr int k_num_hand_connections = 21;
inline constexpr int k_hand_connections[k_num_hand_connections][2] = {
    {0, 1},   {1, 2},   {2, 3},   {3, 4},    // thumb
    {0, 5},   {5, 6},   {6, 7},   {7, 8},    // index
    {5, 9},   {9, 10},  {10, 11}, {11, 12},  // middle
    {9, 13},  {13, 14}, {14, 15}, {15, 16},  // ring
    {13, 17}, {17, 18}, {18, 19}, {19, 20},  // pinky
    {0, 17}};                                // palm

// The models are ONNX files, compiled in: they run on ONNX Runtime.
inline constexpr const char* k_engine_name = "ONNX Runtime";

// Runtime-adjustable settings, mirroring MediaPipe's GestureRecognizerOptions.
struct Options {
    int num_hands = 2;                           // 1 .. k_max_hands
    float min_hand_detection_confidence = 0.5F;  // palm detector score
    float min_hand_presence_confidence = 0.5F;   // landmark presence score
    float min_tracking_confidence = 0.5F;        // IoU for detection/track association
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
    double callback_ms = 0.0;   // glue time of processFrame, never the inference
    double camera_fps = 0.0;    // rate of frames entering the tracker
    double tracking_fps = 0.0;  // rate of frames that completed hand results
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
};

// Expensive (loads models); call off the realtime path. Every anira context of the process
// must ask for the same settings: pass the one the rest of the app uses.
std::unique_ptr<HandTracker> createHandTracker(const anira::ContextConfig& context_config);

}  // namespace anira::models::handtracking
