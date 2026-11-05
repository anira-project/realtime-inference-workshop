// Hand tracking, ready to use: camera frames in, hands out.
//
// Behind this header (HandTracker.cpp and handtracking/): MediaPipe's palm
// detector, hand landmarks and gesture models, running on anira. You need none
// of it: everything a hand does arrives here as a few numbers in [0, 1].
#pragma once

#include <juce_graphics/juce_graphics.h>

#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace hands {

// What a hand does, each feature in [0, 1], seen like a mirror: when you move
// your hand to the right, x grows.
struct HandFeatures {
    float x = 0.5F;         // 0 = left edge, 1 = right edge
    float y = 0.5F;         // 0 = bottom, 1 = top
    float openness = 0.0F;  // 0 = fist, 1 = open palm
    float pinch = 0.0F;     // 0 = thumb and index apart, 1 = touching
    float roll = 0.5F;      // 0 = tilted left, 0.5 = upright, 1 = tilted right
};

// One tracked hand.
struct Hand {
    int id = -1;               // stays the same while the hand is tracked
    bool is_right = false;     // your right hand
    float confidence = 0.0F;   // 0 to 1
    HandFeatures features;
    const char* gesture = "";  // "Open_Palm", "Closed_Fist", "Pointing_Up", "Thumb_Up",
                               // "Thumb_Down", "Victory", "ILoveYou", or "" for none
    // The 21 joints, for drawing: 0 to 1 across the frame, mirrored like the
    // features, y downwards. 0 is the wrist, 4 the thumb tip, 8 the index tip.
    std::array<juce::Point<float>, 21> landmarks{};
};

// Pairs of landmarks that draw the hand's skeleton.
inline constexpr std::array<std::pair<int, int>, 21> k_skeleton{{
    {0, 1}, {1, 2}, {2, 3}, {3, 4},          // thumb
    {0, 5}, {5, 6}, {6, 7}, {7, 8},          // index
    {5, 9}, {9, 10}, {10, 11}, {11, 12},     // middle
    {9, 13}, {13, 14}, {14, 15}, {15, 16},   // ring
    {13, 17}, {17, 18}, {18, 19}, {19, 20},  // pinky
    {0, 17}}};                               // palm

class HandTracker {
public:
    // Loads the models: takes a moment, call it off the audio thread.
    HandTracker();
    ~HandTracker();

    // From the camera's thread, for every frame. Never waits for the inference.
    void pushFrame(const juce::Image& frame);

    // From one thread (the GUI timer): the hands in the newest tracked frame,
    // at most two, the most confident first.
    std::vector<Hand> hands();

    // As of the last hands() call.
    double cameraFps() const;    // frames coming in
    double trackingFps() const;  // frames tracked
    juce::String engine() const;  // the inference engine the models run on

private:
    struct Impl;  // anira and MediaPipe live in HandTracker.cpp
    std::unique_ptr<Impl> m_impl;
};

}  // namespace hands
