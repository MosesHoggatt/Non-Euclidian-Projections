#pragma once
#include "Projection.h"
#include "../math/MathConstants.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  projections/MercatorProjection.h
//
//  The Mercator projection is a CYLINDRICAL CONFORMAL projection, meaning it:
//    - Wraps the sphere in an imaginary cylinder tangent at the equator
//    - Projects sphere points radially outward onto the cylinder
//    - Unrolls the cylinder into a flat rectangle
//
//  The conformal property means angles are locally preserved — a square on
//  the map represents a locally square region on the sphere. This makes it
//  invaluable for navigation: a compass bearing is a straight line on a Mercator map.
//
//  This is the INVERSE direction (flat Mercator coordinates → sphere):
//
//    Mercator coordinates: (longitude, mercatorY)
//      longitude = normalizedX × π              ∈ [−π, π]
//      mercatorY = normalizedY × MERCATOR_SCALE  ∈ [−SCALE, SCALE]
//
//    Inverse Mercator formula:
//      latitude = 2 × atan(exp(mercatorY)) − π/2
//
//    Then standard spherical → Cartesian:
//      x = cos(latitude) × sin(longitude)
//      y = sin(latitude)
//      z = cos(latitude) × cos(longitude)
//
//  The mercatorY formula comes from integrating the secant function,
//  which is what makes the projection conformal. As latitude → ±90°,
//  mercatorY → ±∞, which is why Mercator maps always "cut off" near the poles.
//
//  Properties:
//    ✓ CONFORMAL: preserves angles locally (same property as stereographic)
//    ✓ Rhumb lines (constant compass bearing) are straight lines
//    ✗ SEVERE area distortion: Greenland appears as large as Africa
//    ✗ Cannot show the poles (they are at infinity)
//
//  Visual in the 3D viewer:
//    Horizontal grid lines → latitude circles, evenly spaced in Mercator y but
//      INCREASINGLY COMPRESSED near the equator and SPREAD OUT near poles.
//    Vertical grid lines → meridians, evenly spaced.
//    Notice: near the poles, the latitude circles bunch together on the flat map
//      but are increasingly stretched vertically on the sphere — this is the
//      famous Mercator distortion displayed geometrically.
// ─────────────────────────────────────────────────────────────────────────────

class MercatorProjection : public Projection {
public:
    // Controls latitude coverage. At this value the grid reaches ±78° latitude.
    // Value 2.3 gives: lat = 2×atan(exp(2.3)) − π/2 ≈ ±78.5°
    static constexpr float MERCATOR_Y_SCALE = 2.3f;

    Vector3 mapFlatToSphere(float normalizedX, float normalizedY) const override {
        // Map normalized x to longitude: full 360° coverage
        float longitude = normalizedX * MathConstants::PI;

        // Map normalized y to Mercator Y coordinate
        float mercatorY = normalizedY * MERCATOR_Y_SCALE;

        // INVERSE MERCATOR: recover latitude from Mercator Y.
        // This is the Gudermannian function: gd(y) = 2×atan(e^y) − π/2
        // It arises from the integral of sec(lat), making the map conformal.
        float latitude = 2.0f * std::atan(std::exp(mercatorY)) - MathConstants::HALF_PI;

        float cosLatitude = std::cos(latitude);
        float sinLatitude = std::sin(latitude);

        return {
            cosLatitude * std::sin(longitude),  // x
            sinLatitude,                         // y
            cosLatitude * std::cos(longitude)   // z
        };
    }

    const char* name()      const override { return "Mercator"; }
    ProjectionType type()   const override { return ProjectionType::Mercator; }

    // Orange: "warm", navigation, the famous world map
    Vector3 gridColor() const override { return { 1.0f, 0.5f, 0.1f }; }
};
