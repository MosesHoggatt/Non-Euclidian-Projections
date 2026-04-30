#pragma once
#include "../math/Matrix4.h"
#include "../math/Vector3.h"
#include "../math/MathConstants.h"

// ─────────────────────────────────────────────────────────────────────────────
//  scene/Camera.h
//
//  An orbit camera that rotates around a fixed look-at target.
//  The user controls it by dragging (azimuth + elevation) and scrolling (zoom).
//
//  Coordinate model:
//      - azimuthAngle:   rotation around the world Y axis (horizontal orbit)
//      - elevationAngle: rotation above/below the equator (vertical orbit)
//      - orbitRadius:    distance from camera to the target point
//
//  From these three spherical coordinates the camera position in Cartesian space
//  is derived each frame, and the view matrix is rebuilt via lookAt().
//
//  The camera also owns the perspective projection matrix. Together,
//  view * projection = the MVP matrix prefix shared by all scene objects.
// ─────────────────────────────────────────────────────────────────────────────

class Camera {
public:
    Camera();

    // ── Viewport ──────────────────────────────────────────────────────────────
    // Must be called whenever the canvas is resized to keep the aspect ratio
    // and projection matrix correct.
    void setViewportSize(int widthPixels, int heightPixels);

    // ── Orbit input ───────────────────────────────────────────────────────────
    // Call these from mouse event handlers in main.cpp.

    // Rotates the orbit by a delta in screen pixels.
    // deltaX controls azimuth (left/right), deltaY controls elevation (up/down).
    void orbitByPixelDelta(float deltaX, float deltaY);

    // Zooms in or out by scrolling.
    // A positive deltaY zooms in (decreases orbitRadius), negative zooms out.
    void zoomByScrollDelta(float deltaY);

    // ── Matrix accessors ──────────────────────────────────────────────────────
    // Returns the view matrix: transforms world-space coordinates to camera-space.
    Matrix4 viewMatrix() const;

    // Returns the projection matrix: transforms camera-space to clip-space.
    const Matrix4& projectionMatrix() const { return m_projectionMatrix; }

    // Returns the camera's world-space position (useful for lighting calculations).
    Vector3 worldPosition() const;

    // ── State accessors ───────────────────────────────────────────────────────
    float azimuthAngle()   const { return m_azimuthAngle; }
    float elevationAngle() const { return m_elevationAngle; }
    float orbitRadius()    const { return m_orbitRadius; }

    void  setTarget(const Vector3& target) { m_targetPosition = target; }

private:
    // Spherical coordinates of the camera around the target
    float   m_azimuthAngle;       // radians, horizontal rotation around Y axis
    float   m_elevationAngle;     // radians, vertical angle above the equator
    float   m_orbitRadius;        // distance from camera to target

    // The point the camera always looks at (world origin by default)
    Vector3 m_targetPosition;

    // Perspective projection configuration
    float   m_verticalFovRadians;
    float   m_aspectRatio;
    float   m_nearPlane;
    float   m_farPlane;

    // Cached projection matrix — rebuilt only when the viewport changes
    Matrix4 m_projectionMatrix;

    // Mouse sensitivity constants
    static constexpr float ORBIT_SENSITIVITY  = 0.005f; // radians per pixel
    static constexpr float ZOOM_SENSITIVITY   = 0.1f;   // orbit radius units per scroll tick
    static constexpr float MIN_ORBIT_RADIUS   = 1.2f;
    static constexpr float MAX_ORBIT_RADIUS   = 20.0f;

    // Clamp elevation so the camera never flips past the poles
    static constexpr float MAX_ELEVATION_ANGLE = MathConstants::HALF_PI - 0.05f;

    void rebuildProjectionMatrix();
};
