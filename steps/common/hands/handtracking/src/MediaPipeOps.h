#pragma once

#include <array>
#include <cmath>

#include <handtracking/Types.h>

#include "Geometry.h"

// Stateless reimplementations of the MediaPipe calculators the gesture graph uses.
namespace anira::models::handtracking {

inline constexpr float k_pi = 3.14159265358979F;

inline float sigmoid(float value) {
    return 1.0F / (1.0F + std::exp(-value));
}

// Rotation making from -> to point straight up (MediaPipe convention, y axis down).
inline float rotationFromPoints(Vec2 from, Vec2 to) {
    const float target_angle = k_pi * 0.5F;
    return target_angle - std::atan2(-(to.y - from.y), to.x - from.x);
}

// Shift a center point by shift * size along the rotated y axis.
inline Vec2 shiftAlongRotatedY(Vec2 center, float rotation, float shift, float size) {
    return {center.x - (std::sin(rotation) * shift * size),
            center.y + (std::cos(rotation) * shift * size)};
}

// MediaPipe LandmarksToMatrixCalculator: rotate hand up, origin = wrist, scale = max bbox range; any isotropic input space; 21 x (x, y, z) interleaved.
inline void normalizeLandmarksForEmbedder(const std::array<Vec2, 21>& points,
                                          const std::array<float, 21>& z_values,
                                          float rotation,
                                          std::array<float, 63>& destination) {
    const float cos_r = std::cos(rotation);
    const float sin_r = -std::sin(rotation);  // y axis points down

    std::array<Vec2, 21> rotated;
    for (size_t i = 0; i < 21; ++i) {
        rotated[i] = {(points[i].x * cos_r) - (points[i].y * sin_r),
                      (points[i].y * cos_r) + (points[i].x * sin_r)};
    }

    auto min_x = rotated[0].x, max_x = rotated[0].x;
    auto min_y = rotated[0].y, max_y = rotated[0].y;
    for (const auto& point : rotated) {
        min_x = std::fmin(min_x, point.x);
        max_x = std::fmax(max_x, point.x);
        min_y = std::fmin(min_y, point.y);
        max_y = std::fmax(max_y, point.y);
    }
    const float scale = std::fmax(max_x - min_x, max_y - min_y) + 1e-5F;
    const auto origin = rotated[0];  // object_normalization_origin_offset = 0
    const float origin_z = z_values[0];

    for (size_t i = 0; i < 21; ++i) {
        destination[(i * 3) + 0] = (rotated[i].x - origin.x) / scale;
        destination[(i * 3) + 1] = (rotated[i].y - origin.y) / scale;
        destination[(i * 3) + 2] = (z_values[i] - origin_z) / scale;
    }
}

// Bilinear sample of a transformed square into an interleaved float RGB tensor (out-of-bounds = black). No allocation.
inline void sampleToTensorRGB(const Frame& source,
                              const Affine2D& dest_to_source,
                              int dest_size,
                              float* destination) {
    int r_off = 2, g_off = 1, b_off = 0;  // BGR / BGRA
    switch (source.format) {
        case PixelFormat::RGB:
        case PixelFormat::RGBA: r_off = 0; g_off = 1; b_off = 2; break;
        case PixelFormat::ARGB: r_off = 1; g_off = 2; b_off = 3; break;
        case PixelFormat::BGR:
        case PixelFormat::BGRA: break;
    }

    struct RGB {
        float r = 0.0F, g = 0.0F, b = 0.0F;
    };
    const auto sample = [&](int x, int y) -> RGB {
        if (x < 0 || y < 0 || x >= source.width || y >= source.height)
            return {};
        const std::uint8_t* pixel =
            source.pixels + (static_cast<size_t>(y) * static_cast<size_t>(source.stride)) +
            (static_cast<size_t>(x) * static_cast<size_t>(source.pixel_stride));
        constexpr float inv255 = 1.0F / 255.0F;
        return {static_cast<float>(pixel[r_off]) * inv255,
                static_cast<float>(pixel[g_off]) * inv255,
                static_cast<float>(pixel[b_off]) * inv255};
    };

    size_t i = 0;
    for (int y = 0; y < dest_size; ++y) {
        for (int x = 0; x < dest_size; ++x) {
            const Vec2 source_point = dest_to_source.transform(
                {static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F});
            const float source_x = source_point.x - 0.5F;
            const float source_y = source_point.y - 0.5F;

            const int x0 = static_cast<int>(std::floor(source_x));
            const int y0 = static_cast<int>(std::floor(source_y));
            const float tx = source_x - static_cast<float>(x0);
            const float ty = source_y - static_cast<float>(y0);

            const auto c00 = sample(x0, y0);
            const auto c10 = sample(x0 + 1, y0);
            const auto c01 = sample(x0, y0 + 1);
            const auto c11 = sample(x0 + 1, y0 + 1);

            const auto lerp2d = [tx, ty](float v00, float v10, float v01, float v11) {
                const float top = v00 + ((v10 - v00) * tx);
                const float bottom = v01 + ((v11 - v01) * tx);
                return top + ((bottom - top) * ty);
            };

            destination[i++] = lerp2d(c00.r, c10.r, c01.r, c11.r);
            destination[i++] = lerp2d(c00.g, c10.g, c01.g, c11.g);
            destination[i++] = lerp2d(c00.b, c10.b, c01.b, c11.b);
        }
    }
}

}  // namespace anira::models::handtracking
