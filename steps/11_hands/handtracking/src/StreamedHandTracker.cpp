#include "StreamedHandTracker.h"

#include <algorithm>
#include <cassert>

#include "InferenceConfigs.h"
#include "MediaPipeOps.h"

namespace anira::models::handtracking {

StreamedHandTracker::HandSlot::HandSlot()
    : session(make_landmark_config(), make_context_config())
    , input(static_cast<size_t>(k_landmark_input_size) * k_landmark_input_size * 3) {}

StreamedHandTracker::StreamedHandTracker()
    : m_detector(make_detector_config(), make_context_config())
    , m_embedder(make_embedder_config(), make_context_config())
    , m_classifier(make_classifier_config(), make_context_config())
    , m_anchors(generate_ssd_anchors())
    , m_detector_input(static_cast<size_t>(k_detector_input_size) * k_detector_input_size * 3)
    , m_detector_boxes(static_cast<size_t>(k_num_anchors) * k_num_detection_values)
    , m_detector_scores(k_num_anchors) {
    assert(m_anchors.size() == k_num_anchors);

    m_active_backend = available_backends().front().backend;
    m_state.requestBackend(m_active_backend);

    m_detector.prepare(m_active_backend, detector_deadline_ms(m_active_backend),
                       k_detector_input_size * k_detector_input_size * 3,
                       k_num_anchors * k_num_detection_values, k_camera_frame_rate);
    m_embedder.prepare(m_active_backend, k_gesture_stage_deadline_ms, 63, 128,
                       k_camera_frame_rate * k_max_hands);
    m_classifier.prepare(m_active_backend, k_gesture_stage_deadline_ms, 128, 8,
                         k_camera_frame_rate * k_max_hands);

    // Equal configs -> anira pools the backend processor; the landmark model is loaded once.
    for (auto& slot : m_slots) {
        slot = std::make_unique<HandSlot>();
        slot->session.prepare(m_active_backend, landmark_deadline_ms(m_active_backend),
                              k_landmark_input_size * k_landmark_input_size * 3, 63,
                              k_camera_frame_rate);
    }
}

void StreamedHandTracker::applyBackend(anira::InferenceBackend backend) {
    m_detector.switchBackend(backend, detector_deadline_ms(backend),
                             k_detector_input_size * k_detector_input_size * 3,
                             k_num_anchors * k_num_detection_values, k_camera_frame_rate);
    m_embedder.switchBackend(backend, k_gesture_stage_deadline_ms, 63, 128,
                             k_camera_frame_rate * k_max_hands);
    m_classifier.switchBackend(backend, k_gesture_stage_deadline_ms, 128, 8,
                               k_camera_frame_rate * k_max_hands);

    for (auto& slot : m_slots) {
        slot->session.switchBackend(backend, landmark_deadline_ms(backend),
                                    k_landmark_input_size * k_landmark_input_size * 3, 63,
                                    k_camera_frame_rate);
        slot->active = false;
        slot->lost = false;
        slot->tracking = false;
        slot->last_gesture = -1;
        slot->last_gesture_score = 0.0F;
    }

    m_active_backend = backend;
}

Result StreamedHandTracker::getLatestResult() {
    return m_results.read();
}

void StreamedHandTracker::processFrame(const Frame& frame) {
    const double start_ms = nowMs();
    const Options options = getOptions();

    m_camera_fps.tick(start_ms);

    const auto requested = m_state.requestedBackend();
    if (requested != m_active_backend)
        applyBackend(requested);

    Result& result = m_results.writeBuffer();
    result = Result{};
    result.valid = true;
    result.backend = m_active_backend;

    // Pop side: consume whatever finished since the last frame.
    popDetector(options);
    for (int i = 0; i < k_max_hands; ++i) {
        if (m_slots[static_cast<size_t>(i)]->active)
            popLandmarks(i, options, result);
    }
    popGestureStages();

    // If num_hands was lowered at runtime, drain the extra slots.
    int num_live = 0;
    for (auto& slot : m_slots) {
        if (slot->active && !slot->lost && ++num_live > options.num_hands)
            slot->lost = true;
    }

    // Push side: queue new work for anira's thread pool.
    if (num_live < options.num_hands) {
        pushDetector(frame);
    } else if (m_detector.allPopsServed() && m_detector.hasInFlightResults()) {
        // Detector paused: discard stale in-pipe results so re-acquisition uses fresh frames.
        m_detector.discardInFlightResults();
    }

    for (auto& slot : m_slots) {
        // Free a lost slot only once every scheduled pop is served; discard leftovers so the next hand pairs fresh pushes with fresh results.
        if (slot->active && slot->lost && slot->session.allPopsServed()) {
            slot->session.discardInFlightResults();
            slot->active = false;
            slot->lost = false;
            slot->tracking = false;
            slot->last_gesture = -1;
            slot->last_gesture_score = 0.0F;
        }
        if (slot->active && !slot->lost && slot->session.canPush())
            pushLandmarkCrop(*slot, frame);
    }

    if (result.num_hands > 0)
        m_tracking_fps.tick(start_ms);
    else
        m_tracking_fps.decayIfStale(start_ms);
    result.camera_fps = m_camera_fps.fps;
    result.tracking_fps = m_tracking_fps.fps;

    result.callback_ms = nowMs() - start_ms;

    m_results.publish();
}

void StreamedHandTracker::popDetector(const Options& options) {
    const auto try_pop = [this] {
        float* box_channels[1] = {m_detector_boxes.data()};
        float* score_channels[1] = {m_detector_scores.data()};
        float* const* output_tensors[2] = {box_channels, score_channels};
        size_t num_output_samples[2] = {m_detector_boxes.size(), m_detector_scores.size()};
        const size_t* received =
            m_detector.handler().pop_data(output_tensors, num_output_samples);
        return received[0] >= m_detector_boxes.size();
    };

    m_detector.drainResults(try_pop, [&](const DetectorPushContext& context) {
        std::array<RegionOfInterest, k_max_hands> rois;
        const size_t num_rois =
            decodePalmROIs(m_detector_boxes.data(), m_detector_scores.data(), m_anchors,
                           options.min_hand_detection_confidence, options.num_hands,
                           context.letterbox, rois);

        // Activate free slots for new hands, skipping already-tracked ones.
        for (size_t r = 0; r < num_rois; ++r) {
            const bool duplicate =
                std::any_of(m_slots.begin(), m_slots.end(), [&](const auto& slot) {
                    return slot->active &&
                           roiMatchesTracked(rois[r], slot->roi,
                                             options.min_tracking_confidence);
                });
            if (duplicate)
                continue;

            for (auto& slot : m_slots) {
                if (!slot->active) {
                    slot->active = true;
                    slot->lost = false;
                    slot->tracking = false;
                    slot->roi = rois[r];
                    break;
                }
            }
        }
    });
}

void StreamedHandTracker::popLandmarks(int slot_index, const Options& options, Result& result) {
    HandSlot& slot = *m_slots[static_cast<size_t>(slot_index)];
    const auto input_side = static_cast<float>(k_landmark_input_size);

    const auto try_pop = [&slot] {
        float* landmark_channels[1] = {slot.out_landmarks.data()};
        float* unused_channels[1] = {nullptr};
        float* world_channels[1] = {slot.out_world_landmarks.data()};
        float* const* output_tensors[4] = {landmark_channels, unused_channels, unused_channels,
                                           world_channels};
        size_t num_output_samples[4] = {63, 0, 0, 63};
        const size_t* received = slot.session.handler().pop_data(output_tensors,
                                                                 num_output_samples);
        return received[0] >= 63;
    };

    // Several results may have finished since last frame; only the newest is used.
    bool has_hand = false;
    Hand hand;
    std::array<Vec2, 21> landmarks_px;
    std::array<float, 21> z_values{};
    std::array<float, 63> world_landmarks{};
    float roi_rotation = 0.0F;
    float handedness_right = 0.0F;

    slot.session.drainResults(try_pop, [&](const SlotPushContext& context) {
        // Non-streamable scalar outputs: latest-value semantics via the pre/post processor.
        const float presence = slot.session.pp().get_output(1, 0);

        if (presence < options.min_hand_presence_confidence) {
            slot.lost = true;  // stop pushing; slot frees once pops drain
            return;            // keep draining the remaining results
        }
        slot.lost = false;  // presence recovered while draining
        handedness_right = slot.session.pp().get_output(2, 0);

        const auto frame_width = static_cast<float>(context.frame.width);
        const auto frame_height = static_cast<float>(context.frame.height);

        hand = Hand{};
        hand.slot = slot_index;
        hand.tracking = slot.tracking;
        hand.presence = presence;
        hand.handedness_right = handedness_right;

        const std::array<Vec2, 4> crop_corners{
            Vec2{0.0F, 0.0F}, {input_side, 0.0F}, {input_side, input_side},
            {0.0F, input_side}};
        for (size_t i = 0; i < 4; ++i) {
            const auto corner = context.crop_to_frame.transform(crop_corners[i]);
            hand.roi_corners[i] = {corner.x / frame_width, corner.y / frame_height};
        }

        // Crop pixels -> full-frame coords via the transform captured at push time (latency compensation).
        const float z_scale = context.roi_size / input_side;
        for (size_t i = 0; i < 21; ++i) {
            landmarks_px[i] = context.crop_to_frame.transform(
                {slot.out_landmarks[(i * 3) + 0], slot.out_landmarks[(i * 3) + 1]});
            z_values[i] = slot.out_landmarks[(i * 3) + 2] * z_scale;
            hand.landmarks[i] = {landmarks_px[i].x / frame_width,
                                 landmarks_px[i].y / frame_height};
        }
        world_landmarks = slot.out_world_landmarks;

        slot.roi = roiFromLandmarks(landmarks_px, roi_rotation);
        slot.tracking = true;
        has_hand = true;
    });

    if (!has_hand)
        return;

    hand.gesture = slot.last_gesture;
    hand.gesture_score = slot.last_gesture_score;

    if (m_embedder.canPush())
        pushEmbedder(landmarks_px, z_values, world_landmarks, roi_rotation, handedness_right,
                     slot_index);

    result.hands[result.num_hands++] = hand;
}

void StreamedHandTracker::popGestureStages() {
    // Classifier first, so a fresh embedding can be handed straight to it below.
    const auto try_pop_classifier = [this] {
        float* score_channels[1] = {m_gesture_scores.data()};
        float* const* outputs[1] = {score_channels};
        size_t num_samples[1] = {8};
        const size_t* received = m_classifier.handler().pop_data(outputs, num_samples);
        return received[0] >= 8;
    };

    m_classifier.drainResults(try_pop_classifier, [&](int slot_index) {
        HandSlot& slot = *m_slots[static_cast<size_t>(slot_index)];
        if (slot.active) {
            const auto [gesture, score] = classifyGestureScores(m_gesture_scores);
            slot.last_gesture = gesture;
            slot.last_gesture_score = score;
        }
    });

    const auto try_pop_embedder = [this] {
        float* embedding_channels[1] = {m_embedding.data()};
        float* const* outputs[1] = {embedding_channels};
        size_t num_samples[1] = {128};
        const size_t* received = m_embedder.handler().pop_data(outputs, num_samples);
        return received[0] >= 128;
    };

    m_embedder.drainResults(try_pop_embedder, [&](int slot_index) {
        if (!m_classifier.canPush())
            return;
        const float* in_channels[1] = {m_embedding.data()};
        const float* const* inputs[1] = {in_channels};
        size_t num_input_samples[1] = {128};
        m_classifier.handler().push_data(inputs, num_input_samples);
        m_classifier.recordPush(slot_index);
    });
}

void StreamedHandTracker::pushDetector(const Frame& frame) {
    if (!m_detector.canPush())
        return;

    const auto letterbox = letterboxGeometry(frame);
    sampleToTensorRGB(frame, letterboxDestToSource(letterbox), k_detector_input_size,
                      m_detector_input.data());

    const float* input_channels[1] = {m_detector_input.data()};
    const float* const* input_tensors[1] = {input_channels};
    size_t num_input_samples[1] = {m_detector_input.size()};
    m_detector.handler().push_data(input_tensors, num_input_samples);
    m_detector.recordPush({frame, letterbox});
}

void StreamedHandTracker::pushLandmarkCrop(HandSlot& slot, const Frame& frame) {
    const auto frame_to_crop = roiFrameToCrop(slot.roi);
    const auto crop_to_frame = frame_to_crop.inverted();

    sampleToTensorRGB(frame, crop_to_frame, k_landmark_input_size, slot.input.data());

    const float* input_channels[1] = {slot.input.data()};
    const float* const* input_tensors[1] = {input_channels};
    size_t num_input_samples[1] = {slot.input.size()};
    slot.session.handler().push_data(input_tensors, num_input_samples);
    slot.session.recordPush({frame, crop_to_frame, slot.roi.size});
}

void StreamedHandTracker::pushEmbedder(const std::array<Vec2, 21>& landmarks_px,
                                       const std::array<float, 21>& z_values,
                                       const std::array<float, 63>& world_landmarks,
                                       float roi_rotation,
                                       float handedness_right,
                                       int slot_index) {
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
    const float* const* inputs[3] = {hand_channels, handedness_channels, world_channels};
    size_t num_input_samples[3] = {63, 1, 63};
    m_embedder.handler().push_data(inputs, num_input_samples);
    m_embedder.recordPush(slot_index);
}

}  // namespace anira::models::handtracking
