// Non-Apple platforms: juce::CameraDevice (macOS uses CameraInput.mm).
#include "CameraInput.h"

#include <juce_video/juce_video.h>

namespace hands {

struct CameraInput::Impl : juce::CameraDevice::Listener {
    FrameCallback on_frame;
    ErrorCallback on_error;
    std::unique_ptr<juce::CameraDevice> device;

    void imageReceived(const juce::Image& image) override {
        // The tracker reads the pixels on other threads: hand it a software copy.
        on_frame(image.convertedToFormat(juce::Image::ARGB).createCopy());
    }
};

CameraInput::CameraInput(FrameCallback on_frame, ErrorCallback on_error) : m_impl(std::make_shared<Impl>()) {
    m_impl->on_frame = std::move(on_frame);
    m_impl->on_error = std::move(on_error);
}

CameraInput::~CameraInput() {
    if (m_impl->device != nullptr)
        m_impl->device->removeListener(m_impl.get());
}

void CameraInput::start() {
    if (juce::CameraDevice::getAvailableDevices().isEmpty()) {
        m_impl->on_error("No camera found.");
        return;
    }
    std::weak_ptr<Impl> weak = m_impl;
    juce::CameraDevice::openDeviceAsync(
        0,
        [weak](juce::CameraDevice* device, const juce::String& error) {
            auto impl = weak.lock();
            if (impl == nullptr) {
                delete device;
                return;
            }
            if (device == nullptr) {
                impl->on_error("Cannot open the camera: " + error);
                return;
            }
            impl->device.reset(device);
            impl->device->addListener(impl.get());
        },
        640, 480, 1280, 720, false);
}

}  // namespace hands
