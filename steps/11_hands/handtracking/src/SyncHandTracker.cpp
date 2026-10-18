#include "SyncHandTracker.h"

#include <algorithm>

#include "InferenceConfigs.h"
#include "MediaPipeOps.h"

namespace anira::models::handtracking {

SyncHandTracker::SyncHandTracker()
    : m_detector(make_detector_config())
    , m_landmarks(make_landmark_config())
    , m_embedder(make_embedder_config())
    , m_classifier(make_classifier_config())
    , m_anchors(generate_ssd_anchors())
    , m_detector_input(static_cast<size_t>(k_detector_input_size) * k_detector_input_size * 3)
    , m_detector_boxes(static_cast<size_t>(k_num_anchors) * k_num_detection_values)
    , m_detector_scores(k_num_anchors)
    , m_landmark_input(static_cast<size_t>(k_landmark_input_size) * k_landmark_input_size * 3) {
    m_active_backend = available_backends().front().backend;
    m_state.requestBackend(m_active_backend);
    prepareSessions(m_active_backend, false);

    m_worker = std::thread([this] { run(); });
}

SyncHandTracker::~SyncHandTracker() {
    {
        const std::lock_guard<std::mutex> lock(m_frame_mutex);
        m_running = false;
    }
    m_frame_cv.notify_one();
    m_worker.join();
}

void SyncHandTracker::prepareSessions(anira::InferenceBackend backend, bool switching) {
    const auto prepare_one = [&](SyncSession& session, int values_per_push,
                                 size_t out0_size, double pushes_per_second) {
        if (switching)
            session.switchBackend(backend, values_per_push, out0_size, pushes_per_second);
        else
            session.prepare(backend, values_per_push, out0_size, pushes_per_second);
    };

    prepare_one(m_detector, k_detector_input_size * k_detector_input_size * 3,
                k_num_anchors * k_num_detection_values, k_camera_frame_rate);
    prepare_one(m_landmarks, k_landmark_input_size * k_landmark_input_size * 3, 63,
                k_camera_frame_rate * k_max_hands);
    prepare_one(m_embedder, 63, 128, k_camera_frame_rate * k_max_hands);
    prepare_one(m_classifier, 128, 8, k_camera_frame_rate * k_max_hands);

    for (auto& slot : m_slots)
        slot = SlotState{};
    m_active_backend = backend;
}

void SyncHandTracker::processFrame(const Frame& frame) {
    m_camera_fps.tick(nowMs());
    m_camera_fps_published.store(m_camera_fps.fps, std::memory_order_relaxed);

    {
        const std::lock_guard<std::mutex> lock(m_frame_mutex);
        m_pending_frame = frame;  // refcount-only; overwriting skips to newest
        m_has_frame = true;
    }
    m_frame_cv.notify_one();
}

Result SyncHandTracker::getLatestResult() {
    const std::lock_guard<std::mutex> lock(m_result_mutex);
    return m_latest_result;
}

void SyncHandTracker::run() {
    while (true) {
        Frame frame;
        {
            std::unique_lock<std::mutex> lock(m_frame_mutex);
            m_frame_cv.wait(lock, [this] { return m_has_frame || !m_running; });
            if (!m_running)
                return;
            frame = std::move(m_pending_frame);
            m_pending_frame = Frame{};
            m_has_frame = false;
        }

        const auto requested = m_state.requestedBackend();
        if (requested != m_active_backend)
            prepareSessions(requested, true);

        const Options options = getOptions();
        const double start_ms = nowMs();

        Result result{};
        result.valid = true;
        result.backend = m_active_backend;

        int num_active = 0;
        for (auto& slot : m_slots) {
            if (slot.active && ++num_active > options.num_hands)
                slot = SlotState{};
        }
        num_active = std::min(num_active, options.num_hands);

        if (num_active < options.num_hands)
            runDetector(frame, options);

        for (int i = 0; i < k_max_hands; ++i) {
            if (m_slots[static_cast<size_t>(i)].active)
                processHand(i, frame, options, result);
        }

        const double now = nowMs();
        if (result.num_hands > 0)
            m_tracking_fps.tick(now);
        else
            m_tracking_fps.decayIfStale(now);
        result.camera_fps = m_camera_fps_published.load(std::memory_order_relaxed);
        result.tracking_fps = m_tracking_fps.fps;
        result.callback_ms = now - start_ms;

        {
            const std::lock_guard<std::mutex> lock(m_result_mutex);
            m_latest_result = result;
        }
    }
}

void SyncHandTracker::runDetector(const Frame& frame, const Options& options) {
    const auto letterbox = letterboxGeometry(frame);
    sampleToTensorRGB(frame, letterboxDestToSource(letterbox), k_detector_input_size,
                      m_detector_input.data());

    const float* input_channels[1] = {m_detector_input.data()};
    const float* const* input_tensors[1] = {input_channels};
    size_t num_input_samples[1] = {m_detector_input.size()};
    m_detector.handler().push_data(input_tensors, num_input_samples);

    m_detector.finishPops([this] {
        float* box_channels[1] = {m_detector_boxes.data()};
        float* score_channels[1] = {m_detector_scores.data()};
        float* const* output_tensors[2] = {box_channels, score_channels};
        size_t num_output_samples[2] = {m_detector_boxes.size(), m_detector_scores.size()};
        const size_t* received =
            m_detector.handler().pop_data(output_tensors, num_output_samples);
        return received[0] >= m_detector_boxes.size();
    });

    std::array<RegionOfInterest, k_max_hands> rois;
    const size_t num_rois =
        decodePalmROIs(m_detector_boxes.data(), m_detector_scores.data(), m_anchors,
                       options.min_hand_detection_confidence, options.num_hands, letterbox,
                       rois);

    for (size_t r = 0; r < num_rois; ++r) {
        const bool duplicate =
            std::any_of(m_slots.begin(), m_slots.end(), [&](const SlotState& slot) {
                return slot.active &&
                       roiMatchesTracked(rois[r], slot.roi, options.min_tracking_confidence);
            });
        if (duplicate)
            continue;

        for (auto& slot : m_slots) {
            if (!slot.active) {
                slot.active = true;
                slot.tracking = false;
                slot.roi = rois[r];
                break;
            }
        }
    }
}

void SyncHandTracker::processHand(int slot_index,
                                  const Frame& frame,
                                  const Options& options,
                                  Result& result) {
    SlotState& slot = m_slots[static_cast<size_t>(slot_index)];
    const auto input_side = static_cast<float>(k_landmark_input_size);

    const auto frame_to_crop = roiFrameToCrop(slot.roi);
    const auto crop_to_frame = frame_to_crop.inverted();
    sampleToTensorRGB(frame, crop_to_frame, k_landmark_input_size, m_landmark_input.data());

    const float* input_channels[1] = {m_landmark_input.data()};
    const float* const* input_tensors[1] = {input_channels};
    size_t num_input_samples[1] = {m_landmark_input.size()};
    m_landmarks.handler().push_data(input_tensors, num_input_samples);

    m_landmarks.finishPops([this] {
        float* landmark_channels[1] = {m_out_landmarks.data()};
        float* unused_channels[1] = {nullptr};
        float* world_channels[1] = {m_out_world_landmarks.data()};
        float* const* output_tensors[4] = {landmark_channels, unused_channels, unused_channels,
                                           world_channels};
        size_t num_output_samples[4] = {63, 0, 0, 63};
        const size_t* received =
            m_landmarks.handler().pop_data(output_tensors, num_output_samples);
        return received[0] >= 63;
    });

    // Non-streamable scalar outputs: latest-value semantics via the pre/post processor.
    const float presence = m_landmarks.pp().get_output(1, 0);
    if (presence < options.min_hand_presence_confidence) {
        slot = SlotState{};  // hand gone; no draining needed in sync mode
        return;
    }
    const float handedness_right = m_landmarks.pp().get_output(2, 0);

    const auto frame_width = static_cast<float>(frame.width);
    const auto frame_height = static_cast<float>(frame.height);

    Hand hand;
    hand.slot = slot_index;
    hand.tracking = slot.tracking;
    hand.presence = presence;
    hand.handedness_right = handedness_right;

    const std::array<Vec2, 4> crop_corners{
        Vec2{0.0F, 0.0F}, {input_side, 0.0F}, {input_side, input_side}, {0.0F, input_side}};
    for (size_t i = 0; i < 4; ++i) {
        const auto corner = crop_to_frame.transform(crop_corners[i]);
        hand.roi_corners[i] = {corner.x / frame_width, corner.y / frame_height};
    }

    std::array<Vec2, 21> landmarks_px;
    std::array<float, 21> z_values{};
    const float z_scale = slot.roi.size / input_side;
    for (size_t i = 0; i < 21; ++i) {
        landmarks_px[i] = crop_to_frame.transform(
            {m_out_landmarks[(i * 3) + 0], m_out_landmarks[(i * 3) + 1]});
        z_values[i] = m_out_landmarks[(i * 3) + 2] * z_scale;
        hand.landmarks[i] = {landmarks_px[i].x / frame_width,
                             landmarks_px[i].y / frame_height};
    }

    float roi_rotation = 0.0F;
    slot.roi = roiFromLandmarks(landmarks_px, roi_rotation);
    slot.tracking = true;

    classifyGesture(landmarks_px, z_values, m_out_world_landmarks, roi_rotation,
                    handedness_right, hand);

    result.hands[result.num_hands++] = hand;
}

void SyncHandTracker::classifyGesture(const std::array<Vec2, 21>& landmarks_px,
                                      const std::array<float, 21>& z_values,
                                      const std::array<float, 63>& world_landmarks,
                                      float roi_rotation,
                                      float handedness_right,
                                      Hand& hand) {
    normalizeLandmarksForEmbedder(landmarks_px, z_values, roi_rotation, m_embedder_hand);

    // World landmarks are already metric; same rotation + normalization.
    std::array<Vec2, 21> world_points;
    std::array<float, 21> world_z;
    for (size_t i = 0; i < 21; ++i) {
        world_points[i] = {world_landmarks[(i * 3) + 0], world_landmarks[(i * 3) + 1]};
        world_z[i] = world_landmarks[(i * 3) + 2];
    }
    normalizeLandmarksForEmbedder(world_points, world_z, roi_rotation, m_embedder_world);

    const float* hand_channels[1] = {m_embedder_hand.data()};
    const float* handedness_channels[1] = {&handedness_right};
    const float* world_channels[1] = {m_embedder_world.data()};
    const float* const* embedder_inputs[3] = {hand_channels, handedness_channels,
                                              world_channels};
    size_t embedder_samples[3] = {63, 1, 63};
    m_embedder.handler().push_data(embedder_inputs, embedder_samples);

    m_embedder.finishPops([this] {
        float* embedding_channels[1] = {m_embedding.data()};
        float* const* outputs[1] = {embedding_channels};
        size_t num_samples[1] = {128};
        const size_t* received = m_embedder.handler().pop_data(outputs, num_samples);
        return received[0] >= 128;
    });

    const float* embedding_channels[1] = {m_embedding.data()};
    const float* const* classifier_inputs[1] = {embedding_channels};
    size_t classifier_samples[1] = {128};
    m_classifier.handler().push_data(classifier_inputs, classifier_samples);

    m_classifier.finishPops([this] {
        float* score_channels[1] = {m_gesture_scores.data()};
        float* const* outputs[1] = {score_channels};
        size_t num_samples[1] = {8};
        const size_t* received = m_classifier.handler().pop_data(outputs, num_samples);
        return received[0] >= 8;
    });

    const auto [gesture, score] = classifyGestureScores(m_gesture_scores);
    hand.gesture = gesture;
    hand.gesture_score = score;
}

}  // namespace anira::models::handtracking
