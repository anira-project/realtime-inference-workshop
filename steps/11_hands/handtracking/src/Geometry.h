#pragma once

#include <cmath>

#include <handtracking/Types.h>

// Affine2D composes left-to-right; y axis points down, positive rotation is clockwise on screen.
namespace anira::models::handtracking {

struct Rect {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    float right() const { return x + width; }
    float bottom() const { return y + height; }
    float centreX() const { return x + (width * 0.5F); }
    float centreY() const { return y + (height * 0.5F); }

    bool contains(Vec2 point) const {
        return point.x >= x && point.x < right() && point.y >= y && point.y < bottom();
    }
};

inline float intersectionOverUnion(const Rect& a, const Rect& b) {
    const float overlap_w =
        std::fmin(a.right(), b.right()) - std::fmax(a.x, b.x);
    const float overlap_h =
        std::fmin(a.bottom(), b.bottom()) - std::fmax(a.y, b.y);
    const float intersection =
        (overlap_w > 0.0F && overlap_h > 0.0F) ? overlap_w * overlap_h : 0.0F;
    const float union_area =
        (a.width * a.height) + (b.width * b.height) - intersection;
    return union_area > 0.0F ? intersection / union_area : 0.0F;
}

// x' = m00*x + m01*y + m02, y' = m10*x + m11*y + m12
struct Affine2D {
    float m00 = 1.0F, m01 = 0.0F, m02 = 0.0F;
    float m10 = 0.0F, m11 = 1.0F, m12 = 0.0F;

    static Affine2D translation(float dx, float dy) {
        return {1.0F, 0.0F, dx, 0.0F, 1.0F, dy};
    }

    // This transform followed by `next` (matrix product next * this).
    Affine2D followedBy(const Affine2D& n) const {
        return {(n.m00 * m00) + (n.m01 * m10),
                (n.m00 * m01) + (n.m01 * m11),
                (n.m00 * m02) + (n.m01 * m12) + n.m02,
                (n.m10 * m00) + (n.m11 * m10),
                (n.m10 * m01) + (n.m11 * m11),
                (n.m10 * m02) + (n.m11 * m12) + n.m12};
    }

    Affine2D translated(float dx, float dy) const { return followedBy(translation(dx, dy)); }

    Affine2D rotated(float radians) const {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        return followedBy({c, -s, 0.0F, s, c, 0.0F});
    }

    Affine2D scaled(float factor) const {
        return followedBy({factor, 0.0F, 0.0F, 0.0F, factor, 0.0F});
    }

    Affine2D inverted() const {
        const float det = (m00 * m11) - (m01 * m10);
        const float inv_det = det != 0.0F ? 1.0F / det : 0.0F;
        const float i00 = m11 * inv_det;
        const float i01 = -m01 * inv_det;
        const float i10 = -m10 * inv_det;
        const float i11 = m00 * inv_det;
        return {i00, i01, -((i00 * m02) + (i01 * m12)),
                i10, i11, -((i10 * m02) + (i11 * m12))};
    }

    Vec2 transform(Vec2 point) const {
        return {(m00 * point.x) + (m01 * point.y) + m02,
                (m10 * point.x) + (m11 * point.y) + m12};
    }
};

}  // namespace anira::models::handtracking
