#pragma once
#include "../math/Vector3.h"

// ─────────────────────────────────────────────────────────────────────────────
//  projections/Projection.h
//
//  Abstract base class for all projection types.
//
//  A Projection answers one question:
//    "Given a point (nx, ny) in the flat normalized grid space [-1, 1]²,
//     where does it land on the unit sphere?"
//
//  This is the INVERSE projection direction: flat plane → sphere.
//  We use it this way because we start with a regular 2D grid and want to
//  see how it "drapes" onto the sphere surface under each projection.
//
//  The flat coordinates (nx, ny) ∈ [-1, 1]² are dimensionless.
//  Each derived class maps [-1, 1]² to its own natural parameter space
//  (e.g., EquirectangularProjection maps to [−π, π] × [−π/2, π/2]).
//  The returned Vector3 is ALWAYS a point on the unit sphere (length = 1).
//
//  Projection types:
//    0 — Equirectangular  (direct lat/lon → sphere)
//    1 — Stereographic    (plane through pole → sphere, conformal)
//    2 — Gnomonic         (center of sphere → tangent plane, great circles = lines)
//    3 — Mercator         (cylindrical conformal, navigation-friendly)
// ─────────────────────────────────────────────────────────────────────────────

enum class ProjectionType : int {
    Equirectangular = 0,
    Stereographic   = 1,
    Gnomonic        = 2,
    Mercator        = 3
};

class Projection {
public:
    virtual ~Projection() = default;

    // Maps a normalized flat-space coordinate to a point on the unit sphere.
    // Both nx and ny are in [-1, 1].
    // The returned Vector3 has length exactly 1.0.
    virtual Vector3 mapFlatToSphere(float normalizedX, float normalizedY) const = 0;

    // Human-readable name for the projection
    virtual const char* name() const = 0;

    // Color used for the projected grid lines (RGB in [0, 1])
    virtual Vector3 gridColor() const = 0;

    // Returns the projection type ID
    virtual ProjectionType type() const = 0;
};
