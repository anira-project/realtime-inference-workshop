#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <handtracking/HandTracker.h>

#include <memory>
#include <optional>

#include "CameraInput.h"
#include "HandControls.h"
#include "LatentDecoder.h"

namespace hands {

// Webcam -> anira hand tracking -> latents -> anira decoder -> audio device.
// Without a camera, the video area is a pad: the mouse is the lead hand.
class MainComponent : public juce::Component, private juce::Timer {
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void onCameraFrame(const juce::Image& image);  // capture thread
    void showAudioSettings();
    void padFrom(const juce::MouseEvent& event);

    juce::Rectangle<float> videoBounds() const;
    void paintVideo(juce::Graphics& g, juce::Rectangle<float> area) const;
    void paintHands(juce::Graphics& g, juce::Rectangle<float> video) const;
    void paintLatents(juce::Graphics& g, juce::Rectangle<int> area) const;

    // Tracker before camera: the camera's capture thread calls into it and stops first.
    std::unique_ptr<ht::HandTracker> m_tracker;
    std::unique_ptr<LatentDecoder> m_decoder;
    juce::String m_decoder_error;
    juce::String m_camera_error;

    juce::AudioDeviceManager m_device_manager;
    juce::AudioSourcePlayer m_player;

    juce::SpinLock m_frame_lock;
    juce::Image m_pending_frame;  // guarded by m_frame_lock
    juce::Image m_frame;          // message thread

    HandControls m_controls;
    ht::Result m_result;
    double m_last_tick = 0.0;

    // The pad: the mouse as the lead hand while there is no camera frame
    bool m_pad = false;
    std::optional<HandFeatures> m_pad_features;

    juce::Slider m_volume;
    juce::Slider m_smoothing;
    juce::Slider m_spread;
    juce::Label m_volume_label;
    juce::Label m_smoothing_label;
    juce::Label m_spread_label;
    juce::ToggleButton m_swap_hands{"Swap hands (left hand leads)"};
    juce::ToggleButton m_silence_without_hands{"Silence without hands"};
    juce::TextButton m_audio_settings{"Audio settings..."};
    std::unique_ptr<CameraInput> m_camera;
};

}  // namespace hands
