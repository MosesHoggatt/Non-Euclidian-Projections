#include "Camera.h"
#include "../math/MathConstants.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  scene/Camera.cpp
// ─────────────────────────────────────────────────────────────────────────────

Camera::Camera()
    : m_azimuthAngle(MathConstants::toRadians(30.0f))
    , m_elevationAngle(MathConstants::toRadians(25.0f))
    , m_orbitRadius(5.0f)
    , m_targetPosition(0.0f, 0.0f, 0.0f)
    , m_verticalFovRadians(MathConstants::toRadians(45.0f))
    , m_aspectRatio(1.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(100.0f)
{
    rebuildProjectionMatrix();
}

void Camera::setViewportSize(int widthPixels, int heightPixels) {
    if (heightPixels == 0) return;
    m_aspectRatio = static_cast<float>(widthPixels) / static_cast<float>(heightPixels);
    rebuildProjectionMatrix();
}

void Camera::orbitByPixelDelta(float deltaX, float deltaY) {
    // Horizontal drag rotates azimuth (left/right around the Y axis)
    m_azimuthAngle -= deltaX * ORBIT_SENSITIVITY;

    // Vertical drag rotates elevation (up/down).
    m_elevationAngle += deltaY * ORBIT_SENSITIVITY;

    // Clamp elevation to prevent gimbal flip at the poles
    m_elevationAngle = MathConstants::clamp(
        m_elevationAngle,
        -MAX_ELEVATION_ANGLE,
        +MAX_ELEVATION_ANGLE
    );
}

void Camera::zoomByScrollDelta(float deltaY) {
    // Positive delta scrolls toward the scene (zoom in), negative zooms out
    m_orbitRadius += deltaY * ZOOM_SENSITIVITY;
    m_orbitRadius = MathConstants::clamp(m_orbitRadius, MIN_ORBIT_RADIUS, MAX_ORBIT_RADIUS);
}

Vector3 Camera::worldPosition() const {
    // Convert spherical (azimuth, elevation, radius) → Cartesian (x, y, z).
    //
    // In our convention:
    //   azimuth = 0    → camera on the +Z axis
    //   elevation = 0  → camera on the equatorial plane (y = 0)
    //
    float horizontalRadius = m_orbitRadius * std::cos(m_elevationAngle);
    return {
        m_targetPosition.x + horizontalRadius * std::sin(m_azimuthAngle),
        m_targetPosition.y + m_orbitRadius    * std::sin(m_elevationAngle),
        m_targetPosition.z + horizontalRadius * std::cos(m_azimuthAngle)
    };
}

Matrix4 Camera::viewMatrix() const {
    return Transforms::lookAt(
        worldPosition(),
        m_targetPosition,
        Vector3(0.0f, 1.0f, 0.0f)  // world up axis
    );
}

void Camera::rebuildProjectionMatrix() {
    m_projectionMatrix = Transforms::perspective(
        m_verticalFovRadians,
        m_aspectRatio,
        m_nearPlane,
        m_farPlane
    );
}
