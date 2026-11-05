// Platforms without a camera backend (Linux without OpenCV): the pad plays.
#include "CameraInput.h"

namespace hands {

struct CameraInput::Impl {
    FrameCallback on_frame;
    ErrorCallback on_error;
};

CameraInput::CameraInput(FrameCallback on_frame, ErrorCallback on_error) : m_impl(std::make_shared<Impl>()) {
    m_impl->on_frame = std::move(on_frame);
    m_impl->on_error = std::move(on_error);
}

CameraInput::~CameraInput() = default;

void CameraInput::start() {
    m_impl->on_error("No camera on this platform.");
}

}  // namespace hands
