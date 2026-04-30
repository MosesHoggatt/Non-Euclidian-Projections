#pragma once
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  math/Vector2.h
//
//  A 2-component floating-point vector, used to represent UV texture coordinates
//  and flat 2D grid positions before they are projected onto a 3D surface.
// ─────────────────────────────────────────────────────────────────────────────

struct Vector2 {
    float x, y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const { return { x + other.x, y + other.y }; }
    Vector2 operator-(const Vector2& other) const { return { x - other.x, y - other.y }; }
    Vector2 operator*(float scalar)         const { return { x * scalar,  y * scalar  }; }
    Vector2 operator/(float scalar)         const { return { x / scalar,  y / scalar  }; }

    float length()        const { return std::sqrt(x * x + y * y); }
    Vector2 normalized()  const { float m = length(); return m < 1e-8f ? Vector2{} : *this / m; }
    float dot(const Vector2& other) const { return x * other.x + y * other.y; }

    const float* data() const { return &x; }
};
