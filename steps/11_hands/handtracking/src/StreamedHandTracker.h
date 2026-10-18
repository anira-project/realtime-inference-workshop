#pragma once

#include <anira/anira.h>
#include <array>
#include <memory>
#include <vector>

#include <handtracking/HandTracker.h>

#include "Geometry.h"
#include "StreamedSession.h"
#include "TrackerShared.h"
#include "TripleBuffer.h"

namespace anira::models::handtracking {

// Streamed engine: each model is an anira session in realtime streaming mode driven from the video callback.
// No locks and no heap allocation on the video thread; all buffers preallocated, results via lock-free triple buffer.
class StreamedHandTracker : public HandTracker {
public:
    StreamedHandTracker();

    // Camera thread, non-blocking: pops finished results, pushes new work.
    void processFrame(const Frame& frame) override;

    Result getLatestResult() override;

    void setOptions(const Options& options) override { m_state.setOptions(options); }
    Options getOptions() const override { return m_state.getOptions(); }

    // Applied on the camera thread at the start of the next frame.
    void setBackend(anira::InferenceBackend backend) override {
        m_state.requestBackend(backend);
    }
    anira::InferenceBackend getBackend() const override {
        return m_state.requestedBackend();
    }

private:
    // Must cover anira's computed latency plus headroom; pushes are skipped if a queue fills.
    static constexpr size_t k_queue_capacity = 16;

    // Contexts captured at push time, consumed when the matching result pops.
    struct DetectorPushContext {
        Frame frame;
        LetterboxGeometry letterbox;
    };
    struct SlotPushContext {
        Frame frame;
        Affine2D crop_to_frame;
        float roi_size = 0.0F;  // crop side length in frame pixels (for z scaling)
    };

    struct HandSlot {
        HandSlot();

        StreamedSession<SlotPushContext, k_queue_capacity> session;

        std::vector<float> input;               // preallocated 224x224x3 tensor
        std::array<float, 63> out_landmarks{};  // 21 x (x, y, z) in crop pixels
        std::array<float, 63> out_world_landmarks{};

        bool active = false;    // slot owns a hand (or is draining after loss)
        bool lost = false;      // stop pushing; free the slot once pops drain
        bool tracking = false;  // ROI comes from previous landmarks
        RegionOfInterest roi;

        int last_gesture = -1;  // sticky: updated when the classifier pops
        float last_gesture_score = 0.0F;
    };

    // Camera thread: drains in-flight work, resets tracking, re-prepares every session.
    void applyBackend(anira::InferenceBackend backend);

    // Pop side (non-blocking)
    void popDetector(const Options& options);
    void popLandmarks(int slot_index, const Options& options, Result& result);
    void popGestureStages();

    // Push side (never waits)
    void pushDetector(const Frame& frame);
    void pushLandmarkCrop(HandSlot& slot, const Frame& frame);
    void pushEmbedder(const std::array<Vec2, 21>& landmarks_px,
                      const std::array<float, 21>& z_values,
                      const std::array<float, 63>& world_landmarks,
                      float roi_rotation,
                      float handedness_right,
                      int slot_index);

    // Embedder/classifier are shared by all hands (context = slot index), queues sized for k_max_hands pushes per frame.
    StreamedSession<DetectorPushContext, k_queue_capacity> m_detector;
    StreamedSession<int, k_queue_capacity * k_max_hands> m_embedder;
    StreamedSession<int, k_queue_capacity * k_max_hands> m_classifier;
    std::array<std::unique_ptr<HandSlot>, k_max_hands> m_slots;

    // SSD anchor centers (normalized).
    std::vector<Vec2> m_anchors;

    // Preallocated tensor scratch buffers (camera thread only).
    std::vector<float> m_detector_input;   // 192x192x3
    std::vector<float> m_detector_boxes;   // [2016, 18]
    std::vector<float> m_detector_scores;  // [2016, 1]
    std::array<float, 63> m_embedder_hand{};
    std::array<float, 63> m_embedder_world{};
    std::array<float, 128> m_embedding{};
    std::array<float, 8> m_gesture_scores{};

    // Frame-rate measurement (camera thread only).
    FpsMeter m_camera_fps;
    FpsMeter m_tracking_fps;

    SharedTrackerState m_state;                // options + requested backend
    anira::InferenceBackend m_active_backend;  // camera thread only

    // Published results (camera thread writes, GUI thread reads). Lock-free.
    TripleBuffer<Result> m_results;
};

}  // namespace anira::models::handtracking
