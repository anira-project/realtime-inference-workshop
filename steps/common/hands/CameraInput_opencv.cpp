// Linux: OpenCV's VideoCapture (V4L2 underneath), since JUCE has no camera there.
// macOS and Windows use CameraInput.mm and CameraInput.cpp; WORKSHOP_OPENCV_CAMERA
// builds this file on macOS too, to try it there.
#include "CameraInput.h"

#include <juce_events/juce_events.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

namespace hands {

struct CameraInput::Impl {
    FrameCallback on_frame;
    ErrorCallback on_error;
    std::atomic<bool> running{false};
    std::thread thread;

    void reportError(const juce::String& message) {
        juce::MessageManager::callAsync([callback = on_error, message] { callback(message); });
    }

    // Linux: V4L2 directly. OpenCV's default order tries GStreamer first, and its errors
    // (no frames, camera held by another program) don't say why. MJPG keeps USB webcams
    // at full frame rate at 1280x720; cameras without it keep their own format. Elsewhere,
    // or if V4L2 fails: OpenCV's default backend.
    static void openCamera(cv::VideoCapture& capture) {
#if JUCE_LINUX
        if (capture.open(0, cv::CAP_V4L2)) {
            capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
            return;
        }
#endif
        capture.open(0);
    }

    // Capture thread: opening can take a moment and read() blocks until the next frame.
    void run() {
        cv::VideoCapture capture;
        openCamera(capture);
#if JUCE_MAC
        // OpenCV asks for camera permission on the first open and fails at once instead of
        // waiting for the answer: try again for half a minute while the dialog is up.
        for (int attempt = 0; attempt < 30 && running.load() && !capture.isOpened(); ++attempt) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            capture.open(0);
        }
#endif
        if (!capture.isOpened()) {
            if (running.load())
                reportError("No camera found.");
            return;
        }
        capture.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

        cv::Mat frame, bgra;
        int failures = 0;
        while (running.load()) {
            if (!capture.read(frame) || frame.empty()) {
                if (++failures == 30) {
                    reportError("The camera stopped delivering frames.");
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            failures = 0;

            // JUCE ARGB is BGRA in memory on little-endian machines.
            cv::cvtColor(frame, bgra, frame.channels() == 1 ? cv::COLOR_GRAY2BGRA : cv::COLOR_BGR2BGRA);
            juce::Image image(juce::Image::ARGB, bgra.cols, bgra.rows, false, juce::SoftwareImageType());
            {
                juce::Image::BitmapData data(image, juce::Image::BitmapData::writeOnly);
                for (int y = 0; y < bgra.rows; ++y)
                    std::memcpy(data.getLinePointer(y), bgra.ptr(y), static_cast<size_t>(bgra.cols) * 4);
            }
            on_frame(image);
        }
    }
};

CameraInput::CameraInput(FrameCallback on_frame, ErrorCallback on_error) : m_impl(std::make_shared<Impl>()) {
    m_impl->on_frame = std::move(on_frame);
    m_impl->on_error = std::move(on_error);
}

CameraInput::~CameraInput() {
    m_impl->running = false;
    if (m_impl->thread.joinable())
        m_impl->thread.join();
}

void CameraInput::start() {
    if (m_impl->thread.joinable())
        return;
    m_impl->running = true;
    m_impl->thread = std::thread([impl = m_impl.get()] { impl->run(); });
}

}  // namespace hands
