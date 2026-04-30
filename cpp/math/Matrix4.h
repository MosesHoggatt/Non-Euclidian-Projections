#pragma once
#include "Vector3.h"
#include "Vector2.h"
#include <cmath>
#include <array>

// ─────────────────────────────────────────────────────────────────────────────
//  math/Matrix4.h
//
//  A 4×4 column-major floating-point matrix. Column-major storage matches OpenGL
//  convention — glUniformMatrix4fv can receive our data pointer directly.
//
//  Layout in memory:
//      columns[0] = column 0 = (m[0], m[1], m[2], m[3])
//      columns[1] = column 1 = (m[4], m[5], m[6], m[7])
//      ...
//
//  This means matrix[col][row], following OpenGL/GLSL convention.
// ─────────────────────────────────────────────────────────────────────────────

struct Matrix4 {
    // 16 floats stored in column-major order.
    std::array<float, 16> elements;

    // ── Constructors ──────────────────────────────────────────────────────────

    // Constructs a zero matrix.
    Matrix4() : elements{} {}

    // Direct element initialization (column-major, like GLSL mat4).
    Matrix4(std::array<float, 16> values) : elements(values) {}

    // Returns the identity matrix: the "do nothing" transformation.
    static Matrix4 identity() {
        return Matrix4{{
            1, 0, 0, 0,   // column 0
            0, 1, 0, 0,   // column 1
            0, 0, 1, 0,   // column 2
            0, 0, 0, 1    // column 3
        }};
    }

    // ── Element access ────────────────────────────────────────────────────────
    // Access element at column c, row r.
    float& at(int column, int row)             { return elements[column * 4 + row]; }
    float  at(int column, int row) const       { return elements[column * 4 + row]; }

    // Returns raw pointer — pass directly to glUniformMatrix4fv.
    const float* data() const { return elements.data(); }

    // ── Matrix multiplication ─────────────────────────────────────────────────
    // Combines two transformations into one. Order matters: A * B ≠ B * A.
    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += at(k, row) * other.at(column, k);
                }
                result.at(column, row) = sum;
            }
        }
        return result;
    }

    // ── Vector transformation ─────────────────────────────────────────────────
    // Transforms a 3D position (w=1) or direction (w=0) by this matrix.
    // Used in the vertex shader equivalent for CPU-side operations.
    Vector3 transformPoint(const Vector3& point) const {
        float x = at(0,0)*point.x + at(1,0)*point.y + at(2,0)*point.z + at(3,0);
        float y = at(0,1)*point.x + at(1,1)*point.y + at(2,1)*point.z + at(3,1);
        float z = at(0,2)*point.x + at(1,2)*point.y + at(2,2)*point.z + at(3,2);
        float w = at(0,3)*point.x + at(1,3)*point.y + at(2,3)*point.z + at(3,3);
        if (std::abs(w) > 1e-8f) return { x / w, y / w, z / w };
        return { x, y, z };
    }

    Vector3 transformDirection(const Vector3& direction) const {
        return {
            at(0,0)*direction.x + at(1,0)*direction.y + at(2,0)*direction.z,
            at(0,1)*direction.x + at(1,1)*direction.y + at(2,1)*direction.z,
            at(0,2)*direction.x + at(1,2)*direction.y + at(2,2)*direction.z
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  math/Transforms.h (included here for simplicity)
//
//  Factory functions that construct standard 4×4 transformation matrices.
//  These are the building blocks of 3D rendering: model, view, and projection.
// ─────────────────────────────────────────────────────────────────────────────

namespace Transforms {

    // ── Translation ───────────────────────────────────────────────────────────
    // Moves geometry by (tx, ty, tz) in world space.
    inline Matrix4 translation(float tx, float ty, float tz) {
        Matrix4 m = Matrix4::identity();
        m.at(3, 0) = tx;
        m.at(3, 1) = ty;
        m.at(3, 2) = tz;
        return m;
    }

    inline Matrix4 translation(const Vector3& offset) {
        return translation(offset.x, offset.y, offset.z);
    }

    // ── Scale ─────────────────────────────────────────────────────────────────
    inline Matrix4 scale(float sx, float sy, float sz) {
        Matrix4 m = Matrix4::identity();
        m.at(0, 0) = sx;
        m.at(1, 1) = sy;
        m.at(2, 2) = sz;
        return m;
    }

    inline Matrix4 uniformScale(float factor) {
        return scale(factor, factor, factor);
    }

    // ── Rotation ──────────────────────────────────────────────────────────────
    // Rotates by angleRadians around an arbitrary axis using Rodrigues' formula.
    // The axis must be a unit vector.
    inline Matrix4 rotationAroundAxis(const Vector3& axis, float angleRadians) {
        float cosAngle = std::cos(angleRadians);
        float sinAngle = std::sin(angleRadians);
        float oneMinusCos = 1.0f - cosAngle;

        float ax = axis.x, ay = axis.y, az = axis.z;

        return Matrix4{{
            // column 0
            cosAngle + ax*ax*oneMinusCos,
            ay*ax*oneMinusCos + az*sinAngle,
            az*ax*oneMinusCos - ay*sinAngle,
            0.0f,
            // column 1
            ax*ay*oneMinusCos - az*sinAngle,
            cosAngle + ay*ay*oneMinusCos,
            az*ay*oneMinusCos + ax*sinAngle,
            0.0f,
            // column 2
            ax*az*oneMinusCos + ay*sinAngle,
            ay*az*oneMinusCos - ax*sinAngle,
            cosAngle + az*az*oneMinusCos,
            0.0f,
            // column 3
            0.0f, 0.0f, 0.0f, 1.0f
        }};
    }

    inline Matrix4 rotationX(float angleRadians) {
        return rotationAroundAxis({1, 0, 0}, angleRadians);
    }
    inline Matrix4 rotationY(float angleRadians) {
        return rotationAroundAxis({0, 1, 0}, angleRadians);
    }
    inline Matrix4 rotationZ(float angleRadians) {
        return rotationAroundAxis({0, 0, 1}, angleRadians);
    }

    // ── View matrix (lookAt) ──────────────────────────────────────────────────
    // Constructs the view matrix from the camera's position, a point it looks at,
    // and an up direction. This is the inverse of the camera's world transform.
    //
    // The view matrix transforms world-space points into camera-space (eye-space),
    // where the camera sits at the origin looking down the -Z axis.
    inline Matrix4 lookAt(const Vector3& cameraPosition,
                           const Vector3& targetPosition,
                           const Vector3& worldUp) {
        // Forward vector: direction from camera toward target (pointing into screen)
        Vector3 forwardAxis = (targetPosition - cameraPosition).normalized();

        // Right vector: perpendicular to forward and up, forms the camera's X axis
        Vector3 rightAxis = forwardAxis.cross(worldUp).normalized();

        // True up vector: recomputed to be exactly perpendicular to forward and right
        Vector3 trueUpAxis = rightAxis.cross(forwardAxis);

        // Build a column-major matrix. The upper-left 3×3 is the rotation part;
        // the last column encodes the camera translation.
        return Matrix4{{
            // column 0 (camera right axis)
             rightAxis.x,           trueUpAxis.x,          -forwardAxis.x,         0.0f,
            // column 1 (camera up axis)
             rightAxis.y,           trueUpAxis.y,          -forwardAxis.y,         0.0f,
            // column 2 (negative camera forward = screen depth)
             rightAxis.z,           trueUpAxis.z,          -forwardAxis.z,         0.0f,
            // column 3 (translation: -dot(axis, cameraPosition))
            -rightAxis.dot(cameraPosition),
            -trueUpAxis.dot(cameraPosition),
             forwardAxis.dot(cameraPosition),
             1.0f
        }};
    }

    // ── Perspective projection ─────────────────────────────────────────────────
    // Constructs a perspective projection matrix.
    //
    // verticalFovRadians: the vertical angle of the view frustum
    // aspectRatio: viewport width / height
    // nearPlane: distance to the near clipping plane (must be > 0)
    // farPlane: distance to the far clipping plane (must be > nearPlane)
    //
    // This maps the view frustum into the OpenGL NDC cube [-1,1]^3,
    // producing the perspective foreshortening effect (distant objects look smaller).
    inline Matrix4 perspective(float verticalFovRadians,
                                float aspectRatio,
                                float nearPlane,
                                float farPlane) {
        float tanHalfFov = std::tan(verticalFovRadians * 0.5f);

        // Focal length: how much the frustum is "squeezed" in the Y axis
        float yScale = 1.0f / tanHalfFov;
        float xScale = yScale / aspectRatio;

        // Maps Z from [nearPlane, farPlane] into NDC [-1, 1]
        float zRange     = farPlane - nearPlane;
        float zScale     = -(farPlane + nearPlane) / zRange;
        float zTranslate = -(2.0f * farPlane * nearPlane) / zRange;

        return Matrix4{{
            xScale, 0.0f,   0.0f,   0.0f,   // column 0
            0.0f,   yScale, 0.0f,   0.0f,   // column 1
            0.0f,   0.0f,   zScale, -1.0f,  // column 2: -1 in [3,2] triggers perspective divide
            0.0f,   0.0f,   zTranslate, 0.0f // column 3
        }};
    }

} // namespace Transforms
