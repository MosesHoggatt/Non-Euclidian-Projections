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
    // Call once per frame (or whenever the camera moves) before reprojection.
    void setCenter(const Vector3& cameraFacingPoint) {
        m_center = cameraFacingPoint.normalized();
    }

    // Inverse stereographic centered at m_center.
    // nx, ny ∈ [-1, 1] (flat grid normalised coords)
    Vector3 mapFlatToSphere(float nx, float ny) const override {
        // Scale the flat plane so the grid covers a useful portion of the sphere.
        // 2.5 gives roughly the same coverage as the default stereographic.
        const float scale = 2.5f;
        float u = nx * scale;
        float v = ny * scale;
        float r2 = u * u + v * v;

        // Inverse stereographic about the north pole (0,1,0):
        //   P = ( 2u/(1+r²),  (1-r²)/(1+r²),  2v/(1+r²) )
        float denom = 1.0f + r2;
        Vector3 p(2.0f * u / denom, (1.0f - r2) / denom, 2.0f * v / denom);

        // Rotate (0,1,0) → m_center using Rodrigues' formula so the projection
        // is centred on the camera-facing point instead of the north pole.
        return rotateNorthToCenter(p);
    }

    const char* name() const override { return "View Adaptive"; }
    Vector3     gridColor() const override { return Vector3(0.85f, 0.55f, 1.0f); } // violet
    ProjectionType type() const override { return ProjectionType::ViewAdaptive; }

private:
    Vector3 m_center{0.0f, 1.0f, 0.0f}; // default: north pole

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
