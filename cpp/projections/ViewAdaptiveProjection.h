#pragma once
#include "Projection.h"
#include "../math/MathConstants.h"
#include "../math/Matrix4.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  ViewAdaptiveProjection
//
//  A stereographic projection dynamically centered on the camera's facing point.
//
//  The standard stereographic projects from the south pole (0,-1,0) onto the
//  equatorial plane. Here we generalise: given the unit vector `center`
//  (the sphere point directly facing the camera), we build a rotation from
//  (0,1,0) → center and apply it to every projected point. This keeps the
//  grid squares nearest the viewer as the least-distorted (conformal property),
//  and the projection covers the entire sphere.
//
//  Updated every frame via setCenter() before reprojectGridToGPU() is called.
// ─────────────────────────────────────────────────────────────────────────────

class ViewAdaptiveProjection : public Projection {
public:
    // Call once per frame before reprojection.
    // Stores the camera facing direction for distortion centering AND
    // computes a flat-space scroll offset from the camera's spherical angle.
    // This split means:
    //   • The stereographic distortion minimum stays at the camera center.
    //   • The grid coordinates travel / shift as the camera orbits, so it
    //     feels like moving through a fixed non-Euclidean lattice rather than
    //     having a pattern painted on a window in front of you.
    void setCenter(const Vector3& cameraFacingPoint) {
        m_center = cameraFacingPoint.normalized();

        // Convert facing direction to spherical angles, then map to [-1, 1]
        // flat-space offsets.  These scroll the grid as the camera moves.
        float theta = std::atan2(m_center.x, m_center.z);                          // [-π, π]
        float phi   = std::asin(std::clamp(m_center.y, -1.0f, 1.0f));              // [-π/2, π/2]
        m_offsetX   =  theta / MathConstants::PI;                                   // [-1, 1]
        m_offsetY   =  phi   / MathConstants::HALF_PI;                              // [-1, 1]
    }

    // Inverse stereographic centred at m_center with a travel offset applied.
    // nx, ny ∈ [-1, 1] (flat grid normalised coords)
    Vector3 mapFlatToSphere(float nx, float ny) const override {
        // Shift the flat coordinate by the camera's scroll offset so grid lines
        // slide through the scene as the camera moves.
        const float scale = 2.5f;
        float u = (nx + m_offsetX) * scale;
        float v = (ny + m_offsetY) * scale;
        float r2 = u * u + v * v;

        // Inverse stereographic about the north pole (0,1,0):
        //   P = ( 2u/(1+r²),  (1-r²)/(1+r²),  2v/(1+r²) )
        float denom = 1.0f + r2;
        Vector3 p(2.0f * u / denom, (1.0f - r2) / denom, 2.0f * v / denom);

        // Rotate (0,1,0) → m_center: keeps the distortion minimum at camera center
        // even though the grid itself has scrolled.
        return rotateNorthToCenter(p);
    }

    const char* name() const override { return "View Adaptive"; }
    Vector3     gridColor() const override { return Vector3(0.85f, 0.55f, 1.0f); } // violet
    ProjectionType type() const override { return ProjectionType::ViewAdaptive; }

private:
    Vector3 m_center{0.0f, 1.0f, 0.0f}; // default: north pole
    float   m_offsetX{0.0f};             // flat-space scroll: camera azimuth  / π
    float   m_offsetY{0.0f};             // flat-space scroll: camera elevation / (π/2)

    // Rotate vector v such that the Y axis maps to m_center.
    Vector3 rotateNorthToCenter(const Vector3& v) const {
        const Vector3 north(0.0f, 1.0f, 0.0f);

        // If center ≈ north, identity
        float dot = north.dot(m_center);
        if (dot >  0.9999f) return v;

        // If center ≈ south pole, flip Y
        if (dot < -0.9999f) return Vector3(v.x, -v.y, v.z);

        // General case: Rodrigues rotation
        Vector3 axis  = north.cross(m_center).normalized();
        float   angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));
        Matrix4 rot   = Transforms::rotationAroundAxis(axis, angle);
        return rot.transformDirection(v);
    }
};
