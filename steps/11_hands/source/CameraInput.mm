#include "CameraInput.h"

#include <juce_events/juce_events.h>

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>

#include <cstring>

namespace hands {
// What the Objective-C delegate calls; CameraInput::Impl is private to CameraInput.
struct FrameSink {
    virtual ~FrameSink() = default;
    virtual void deliver(CMSampleBufferRef sample_buffer) = 0;
};
}  // namespace hands

@interface RaveHandsCaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property(nonatomic, assign) hands::FrameSink* owner;
@end

namespace hands {

struct CameraInput::Impl : FrameSink {
    FrameCallback on_frame;
    ErrorCallback on_error;
    AVCaptureSession* session = nil;
    RaveHandsCaptureDelegate* delegate = nil;
    dispatch_queue_t queue = nullptr;

    void reportError(const juce::String& message) {
        juce::MessageManager::callAsync([callback = on_error, message] { callback(message); });
    }

    void open() {
        AVCaptureDevice* device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
        if (device == nil) {
            reportError("No camera found.");
            return;
        }

        NSError* error = nil;
        AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
        if (input == nil) {
            reportError(juce::String("Cannot open the camera: ") + error.localizedDescription.UTF8String);
            return;
        }

        session = [[AVCaptureSession alloc] init];
        if ([session canSetSessionPreset:AVCaptureSessionPreset1280x720])
            session.sessionPreset = AVCaptureSessionPreset1280x720;
        if ([session canAddInput:input])
            [session addInput:input];

        AVCaptureVideoDataOutput* output = [[AVCaptureVideoDataOutput alloc] init];
        output.videoSettings = @{(id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};
        output.alwaysDiscardsLateVideoFrames = YES;
        delegate = [[RaveHandsCaptureDelegate alloc] init];
        delegate.owner = this;
        queue = dispatch_queue_create("io.github.anira-project.workshop-hands.camera", DISPATCH_QUEUE_SERIAL);
        [output setSampleBufferDelegate:delegate queue:queue];
        if ([session canAddOutput:output])
            [session addOutput:output];

        // startRunning blocks until the session is up; keep it off the message thread.
        AVCaptureSession* running = session;
        dispatch_async(queue, ^{
          [running startRunning];
        });
    }

    void close() {
        if (session == nil)
            return;
        [session stopRunning];
        for (AVCaptureOutput* output in session.outputs)
            if ([output isKindOfClass:[AVCaptureVideoDataOutput class]])
                [(AVCaptureVideoDataOutput*)output setSampleBufferDelegate:nil queue:nil];
        dispatch_sync(queue, ^{});  // drain a callback in flight
        delegate.owner = nullptr;
        session = nil;
        delegate = nil;
    }

    void deliver(CMSampleBufferRef sample_buffer) override {
        CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample_buffer);
        if (pixels == nullptr)
            return;

        CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
        const auto width = static_cast<int>(CVPixelBufferGetWidth(pixels));
        const auto height = static_cast<int>(CVPixelBufferGetHeight(pixels));
        const auto row_bytes = CVPixelBufferGetBytesPerRow(pixels);
        const auto* base = static_cast<const std::uint8_t*>(CVPixelBufferGetBaseAddress(pixels));

        // JUCE ARGB is BGRA in memory on little-endian machines, the camera's layout.
        juce::Image image(juce::Image::ARGB, width, height, false, juce::SoftwareImageType());
        {
            juce::Image::BitmapData data(image, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < height; ++y)
                std::memcpy(data.getLinePointer(y), base + static_cast<size_t>(y) * row_bytes, static_cast<size_t>(width) * 4);
        }
        CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);

        on_frame(image);
    }
};

}  // namespace hands

@implementation RaveHandsCaptureDelegate
- (void)captureOutput:(AVCaptureOutput*)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection*)connection {
    if (self.owner != nullptr)
        self.owner->deliver(sampleBuffer);
}
@end

namespace hands {

CameraInput::CameraInput(FrameCallback on_frame, ErrorCallback on_error) : m_impl(std::make_shared<Impl>()) {
    m_impl->on_frame = std::move(on_frame);
    m_impl->on_error = std::move(on_error);
}

CameraInput::~CameraInput() {
    m_impl->close();
}

void CameraInput::start() {
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]) {
        case AVAuthorizationStatusAuthorized:
            m_impl->open();
            return;
        case AVAuthorizationStatusNotDetermined: {
            // The answer may arrive after this CameraInput is gone.
            std::weak_ptr<Impl> weak = m_impl;
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                                     completionHandler:^(BOOL granted) {
                                       juce::MessageManager::callAsync([weak, granted] {
                                           if (auto impl = weak.lock()) {
                                               if (granted)
                                                   impl->open();
                                               else
                                                   impl->reportError("Camera access denied.");
                                           }
                                       });
                                     }];
            return;
        }
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
            m_impl->reportError("Camera access denied: allow it in System Settings > Privacy & Security > Camera.");
            return;
    }
}

}  // namespace hands
