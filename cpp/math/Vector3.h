#pragma once
#include <cmath>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
//  math/Vector3.h
//
//  A 3-component floating-point vector used throughout the engine for positions,
//  directions, and normals. All operations are defined inline for performance.
//  No external dependencies — this file is entirely self-contained.
// ─────────────────────────────────────────────────────────────────────────────

struct Vector3 {
    float x, y, z;

    // ── Constructors ──────────────────────────────────────────────────────────
    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    explicit Vector3(float uniformValue) : x(uniformValue), y(uniformValue), z(uniformValue) {}

    // ── Arithmetic operators ──────────────────────────────────────────────────
    Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
    Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
    Vector3 operator*(float scalar)         const { return { x * scalar,  y * scalar,  z * scalar  }; }
    Vector3 operator/(float scalar)         const { return { x / scalar,  y / scalar,  z / scalar  }; }
    Vector3 operator-()                     const { return { -x, -y, -z }; }

    Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vector3& operator*=(float scalar)         { x *= scalar;  y *= scalar;  z *= scalar;  return *this; }

    bool operator==(const Vector3& other) const { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Vector3& other) const { return !(*this == other); }

    // ── Geometric operations ──────────────────────────────────────────────────

    // Returns the squared magnitude — cheaper than length() when only comparing distances.
    float lengthSquared() const { return x * x + y * y + z * z; }

    // Returns the Euclidean length of the vector.
    float length() const { return std::sqrt(lengthSquared()); }

    // Returns a unit vector pointing in the same direction.
    // Throws if the vector is zero-length (degenerate case).
    Vector3 normalized() const {
        float magnitude = length();
        if (magnitude < 1e-8f) return Vector3(0.0f, 0.0f, 0.0f);
        return *this / magnitude;
    }

    // Dot product: measures how parallel two vectors are.
    // Result is  cos(angle) * |a| * |b|.
    float dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // Cross product: returns a vector perpendicular to both inputs.
    // The resulting vector follows the right-hand rule.
    Vector3 cross(const Vector3& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    // Linear interpolation between this vector and a target vector.
    // At t=0, returns *this; at t=1, returns target.
    Vector3 lerp(const Vector3& target, float t) const {
        return *this + (target - *this) * t;
    }

    // Returns the raw float pointer — used for passing data to OpenGL.
    const float* data() const { return &x; }
};

// Allows scalar * vector (commutative multiplication)
inline Vector3 operator*(float scalar, const Vector3& vector) {
    return vector * scalar;
}
