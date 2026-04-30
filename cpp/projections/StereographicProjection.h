#pragma once
#include "Projection.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  projections/StereographicProjection.h
//
//  The stereographic projection projects the sphere onto a flat plane by
//  drawing a straight line from the SOUTH POLE through each sphere point
//  and recording where that line hits the equatorial plane (y = 0).
//
//  This is the INVERSE direction (flat plane → sphere):
//
//    Given flat point (X, Z) on the equatorial plane:
//
//      r² = X² + Z²
//
//      sphereX = 2X / (1 + r²)
//      sphereY = (1 − r²) / (1 + r²)          ← ranges from +1 (north) to −1 (south)
//      sphereZ = 2Z / (1 + r²)
//
//  Where (X, Z) = normalizedX × SCALE, normalizedY × SCALE
//
//  Properties:
//    ✓ CONFORMAL: preserves local angles everywhere — grid squares on the
//      flat plane appear as "square" shapes on the sphere (though scaled)
//    ✗ NOT equal-area: regions near the south pole appear vastly enlarged
//    ✓ Circles on the sphere map to circles (or lines) on the flat plane
//
//  Visual in the 3D viewer:
//    Grid squares look "square" near the north pole (center of projection).
//    As we move toward the south pole, the squares grow enormously in size.
//    This distortion demonstrates why no flat map can be both conformal AND equal-area.
//
//  The scale factor controls how much of the sphere is covered:
//    SCALE = 1.0 → center third of sphere (about 53° from north pole)
//    SCALE = 2.0 → covers to about 75° south latitude
// ─────────────────────────────────────────────────────────────────────────────

class StereographicProjection : public Projection {
public:
    // How much of the flat plane the normalized [-1, 1] range spans.
    // Increasing this shows more of the sphere (approaching the south pole).
    static constexpr float FLAT_PLANE_SCALE = 2.5f;

    Vector3 mapFlatToSphere(float normalizedX, float normalizedY) const override {
        // Scale normalized coordinates to flat projection plane coordinates
        float flatX = normalizedX * FLAT_PLANE_SCALE;
        float flatZ = normalizedY * FLAT_PLANE_SCALE;

        // r² = distance² from the center of the flat plane
        float r2 = flatX * flatX + flatZ * flatZ;

        // The stereographic inverse formula (projection from south pole)
        float inverseDenominator = 1.0f / (1.0f + r2);

        return {
            2.0f * flatX * inverseDenominator,   // x
            (1.0f - r2) * inverseDenominator,     // y  (1 at center, -1 at infinity)
            2.0f * flatZ * inverseDenominator     // z
        };
        // Note: this is always a unit vector (see header for derivation)
    }

    const char* name()      const override { return "Stereographic"; }
    ProjectionType type()   const override { return ProjectionType::Stereographic; }

    // Cyan: "cool", radiating outward from center
    Vector3 gridColor() const override { return { 0.2f, 0.9f, 1.0f }; }
};
