#pragma once

#include <juce_graphics/juce_graphics.h>

#include <functional>
#include <memory>

namespace hands {

// Webcam frames as ARGB software images (BGRA bytes in memory, unmirrored).
// macOS: AVFoundation video data output (JUCE's CameraDevice delivers still captures there,
// too slow for tracking). Elsewhere: juce::CameraDevice.
class CameraInput {
public:
    using FrameCallback = std::function<void(const juce::Image&)>;  // capture thread
    using ErrorCallback = std::function<void(const juce::String&)>;  // message thread

    CameraInput(FrameCallback on_frame, ErrorCallback on_error);
    ~CameraInput();  // stops capture; no frame callback runs after it returns

    // Asks for camera permission if needed, then opens the default camera.
    void start();

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

}  // namespace hands
