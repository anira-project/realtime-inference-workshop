#pragma once

#include <cstdint>
#include <memory>

namespace anira::models::handtracking {

struct Vec2 {
    float x = 0.0F;
    float y = 0.0F;
};

// Byte order in memory, independent of the host's naming convention.
enum class PixelFormat { RGB, BGR, RGBA, BGRA, ARGB };

// The tracker may retain a frame beyond the call: `pixels` must stay valid while `keep_alive` holds a reference. Copies are refcount-only.
struct Frame {
    const std::uint8_t* pixels = nullptr;
    int width = 0;
    int height = 0;
    int stride = 0;        // bytes per row
    int pixel_stride = 0;  // bytes per pixel (3 or 4)
    PixelFormat format = PixelFormat::BGRA;
    std::shared_ptr<const void> keep_alive;

    bool isValid() const { return pixels != nullptr && width > 0 && height > 0; }
};

}  // namespace anira::models::handtracking
