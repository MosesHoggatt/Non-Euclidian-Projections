#pragma once
#include "Projection.h"
#include "../math/MathConstants.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  projections/EquirectangularProjection.h
//
//  The equirectangular projection (also called "plate carrée") is the simplest
//  mapping: longitude is proportional to x, latitude is proportional to y.
//
//  Flat → Sphere mapping:
//    longitude = normalizedX × π         ∈ [−π,    π  ]
//    latitude  = normalizedY × (π/2)     ∈ [−π/2,  π/2]
//
//    x = cos(latitude) × sin(longitude)
//    y = sin(latitude)
//    z = cos(latitude) × cos(longitude)
//
//  Properties:
//    ✗ Neither conformal (angle-preserving) nor equal-area
//    ✓ Grid lines map directly to lat/lon lines — a "standard" globe grid
//    ✗ Severe area distortion at poles (a 1×1 grid square at 80°N spans the
//      same flat area as one at the equator, but covers a much smaller region)
//
//  Visual in the 3D viewer:
//    Horizontal grid lines → latitude circles (parallels)
//    Vertical grid lines   → longitude meridians
//    Meridians converge at the poles, showing the distortion.
// ─────────────────────────────────────────────────────────────────────────────

class EquirectangularProjection : public Projection {
public:
    Vector3 mapFlatToSphere(float normalizedX, float normalizedY) const override {
        // Map normalized [-1, 1] to angular ranges
        float longitude = normalizedX * MathConstants::PI;
        float latitude  = normalizedY * MathConstants::HALF_PI;

        float cosLatitude = std::cos(latitude);
        float sinLatitude = std::sin(latitude);

        // Spherical → Cartesian (Y-up convention, same as SphereGenerator)
        return {
            cosLatitude * std::sin(longitude),  // x
            sinLatitude,                         // y
            cosLatitude * std::cos(longitude)   // z
        };
    }

    const char* name()      const override { return "Equirectangular"; }
    ProjectionType type()   const override { return ProjectionType::Equirectangular; }

    // Gold: classic, familiar, "reference" projection
    Vector3 gridColor() const override { return { 1.0f, 0.85f, 0.2f }; }
};
