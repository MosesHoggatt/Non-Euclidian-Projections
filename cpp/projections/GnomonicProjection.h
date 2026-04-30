#pragma once
#include "Projection.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  projections/GnomonicProjection.h
//
//  The gnomonic projection projects from the CENTER of the sphere onto a
//  plane tangent to the north pole. Every straight line on the flat plane
//  corresponds to a GREAT CIRCLE on the sphere — this is its defining property.
//
//  This is the INVERSE direction (flat tangent plane → sphere):
//
//    The tangent plane touches the sphere at the north pole (0, 1, 0).
//    A point (X, Z) on this plane is at world position (X, 1, Z).
//    The ray from the sphere's center (0, 0, 0) through (X, 1, Z) intersects
//    the sphere at:
//
//      length = sqrt(X² + 1 + Z²)
//
//      sphereX = X / length
//      sphereY = 1 / length      ← always positive: only northern hemisphere visible
//      sphereZ = Z / length
//
//  Properties:
//    ✓ GREAT CIRCLES appear as STRAIGHT LINES — used in aviation route planning
//    ✗ Only covers one hemisphere (less than half the sphere)
//    ✗ Severe distortion near the horizon (equator)
//    ✗ NOT conformal, NOT equal-area
//
//  Visual in the 3D viewer:
//    The straight grid lines in flat space become great circle arcs on the sphere.
//    Near the north pole, the grid is relatively undistorted.
//    Near the equator (horizon of this projection), straight lines "bow outward"
//    and the grid cells grow enormous — demonstrating the gnomonic's severe limits.
//
//  The scale factor controls how far from the north pole we can see:
//    SCALE = 1.0 → covers about 45° from north pole
//    SCALE = 1.7 → covers about 60° from north pole (approaching equator)
// ─────────────────────────────────────────────────────────────────────────────

class GnomonicProjection : public Projection {
public:
    // At SCALE = 1.7 the grid corners reach ~59° from the north pole.
    // Beyond ~90°, the tangent plane projection diverges (pointing away from sphere).
    static constexpr float FLAT_PLANE_SCALE = 1.7f;

    Vector3 mapFlatToSphere(float normalizedX, float normalizedY) const override {
        // Scale to tangent plane coordinates
        float tangentX = normalizedX * FLAT_PLANE_SCALE;
        float tangentZ = normalizedY * FLAT_PLANE_SCALE;

        // The point on the tangent plane is (tangentX, 1, tangentZ).
        // Dividing by its distance from the origin gives the sphere point.
        float distanceFromOrigin = std::sqrt(tangentX * tangentX + 1.0f + tangentZ * tangentZ);

        return {
            tangentX / distanceFromOrigin,  // x
            1.0f    / distanceFromOrigin,   // y (always > 0: northern hemisphere only)
            tangentZ / distanceFromOrigin   // z
        };
    }

    const char* name()      const override { return "Gnomonic"; }
    ProjectionType type()   const override { return ProjectionType::Gnomonic; }

    // Green: "geometric", straight lines, great circle paths
    Vector3 gridColor() const override { return { 0.3f, 1.0f, 0.45f }; }
};
