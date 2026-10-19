#pragma once

#include <anira/anira.h>
#include <array>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <handtracking/HandTracker.h>

#include "TrackerShared.h"

namespace anira::models::handtracking {

// One anira session driven synchronously: in non-realtime mode pop_data blocks until the result arrives.
class SyncSession {
public:
    explicit SyncSession(anira::InferenceConfig config)
        : m_config(std::move(config)), m_pp(m_config), m_handler(m_pp, m_config, make_context_config()) {}

    anira::InferenceHandler& handler() { return m_handler; }
    anira::PrePostProcessor& pp() { return m_pp; }  // non-streamable tensors

    void prepare(anira::InferenceBackend backend,
                 int values_per_push,
                 size_t out0_size,
                 double pushes_per_second) {
        m_config.m_max_inference_time = k_sync_deadline_ms;
        m_handler.set_inference_backend(backend);
        m_handler.prepare(anira::HostConfig(
            static_cast<float>(values_per_push),
            static_cast<float>(values_per_push) * static_cast<float>(pushes_per_second)));
        m_handler.set_non_realtime(true);
        m_primed_skips = m_handler.get_latency(0) / out0_size;
    }

    void switchBackend(anira::InferenceBackend backend,
                       int values_per_push,
                       size_t out0_size,
                       double pushes_per_second) {
        m_handler.reset();
        prepare(backend, values_per_push, out0_size, pushes_per_second);
    }

    // Call right after handler().push_data(): pops until that push's result arrived (also drains the one-time latency-priming zeros).
    template <typename PopFn>
    void finishPops(PopFn&& pop) {
        for (size_t i = 0; i <= m_primed_skips; ++i) {
            [[maybe_unused]] const bool popped = pop();
            assert(popped);  // non-realtime pops cannot underrun
        }
        m_primed_skips = 0;
    }

private:
    anira::InferenceConfig m_config;
    anira::PrePostProcessor m_pp;
    anira::InferenceHandler m_handler;
    size_t m_primed_skips = 0;
};

// Synchronous engine: the whole chain runs on one worker thread; the camera callback only mailboxes the newest frame (latest frame wins).
class SyncHandTracker : public HandTracker {
public:
    SyncHandTracker();
    ~SyncHandTracker() override;

    // Camera thread: stores the frame for the worker, never blocks on inference.
    void processFrame(const Frame& frame) override;

    Result getLatestResult() override;

    void setOptions(const Options& options) override { m_state.setOptions(options); }
    Options getOptions() const override { return m_state.getOptions(); }

    void setBackend(anira::InferenceBackend backend) override {
        m_state.requestBackend(backend);
    }
    anira::InferenceBackend getBackend() const override {
        return m_state.requestedBackend();
    }

private:
    struct SlotState {
        bool active = false;
        bool tracking = false;  // ROI comes from previous landmarks
        RegionOfInterest roi;
    };

    void run();  // worker thread main loop
    void prepareSessions(anira::InferenceBackend backend, bool switching);
    void runDetector(const Frame& frame, const Options& options);
    void processHand(int slot_index, const Frame& frame, const Options& options,
                     Result& result);
    void classifyGesture(const std::array<Vec2, 21>& landmarks_px,
                         const std::array<float, 21>& z_values,
                         const std::array<float, 63>& world_landmarks,
                         float roi_rotation,
                         float handedness_right,
                         Hand& hand);

    SyncSession m_detector;
    SyncSession m_landmarks;
    SyncSession m_embedder;
    SyncSession m_classifier;

    std::array<SlotState, k_max_hands> m_slots;
    std::vector<Vec2> m_anchors;

    // Tensor scratch buffers (worker thread only).
    std::vector<float> m_detector_input;   // 192x192x3
    std::vector<float> m_detector_boxes;   // [2016, 18]
    std::vector<float> m_detector_scores;  // [2016, 1]
    std::vector<float> m_landmark_input;   // 224x224x3
    std::array<float, 63> m_out_landmarks{};
    std::array<float, 63> m_out_world_landmarks{};
    std::array<float, 63> m_embedder_hand{};
    std::array<float, 63> m_embedder_world{};
    std::array<float, 128> m_embedding{};
    std::array<float, 8> m_gesture_scores{};

    SharedTrackerState m_state;                // options + requested backend
    anira::InferenceBackend m_active_backend;  // worker thread only

    // Latest-frame mailbox (camera thread -> worker).
    std::mutex m_frame_mutex;
    std::condition_variable m_frame_cv;
    Frame m_pending_frame;
    bool m_has_frame = false;
    bool m_running = true;

    // Camera rate is measured on the camera thread and read by the worker, hence atomic.
    FpsMeter m_camera_fps;  // camera thread only (tick)
    std::atomic<double> m_camera_fps_published{0.0};
    FpsMeter m_tracking_fps;

    // Published result (worker writes, GUI reads).
    std::mutex m_result_mutex;
    Result m_latest_result;

    std::thread m_worker;  // last member: joins in dtor after m_running flips
};

}  // namespace anira::models::handtracking
