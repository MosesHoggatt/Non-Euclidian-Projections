#pragma once
#include "Mesh.h"

// ─────────────────────────────────────────────────────────────────────────────
//  geometry/SphereGenerator.h
//
//  Generates a UV sphere mesh — a sphere parameterized by latitude and longitude.
//
//  Parameterization:
//      longitude ∈ [0, 2π]   — angle around the Y axis (like Earth's longitudes)
//      latitude  ∈ [-π/2, π/2] — angle from the equator (like Earth's latitudes)
//
//  This parameterization is the foundation of equirectangular projection:
//  a flat (longitude, latitude) 2D grid maps directly onto the sphere surface.
//
//  Vertex position formula:
//      x = radius * cos(latitude) * sin(longitude)
//      y = radius * sin(latitude)
//      z = radius * cos(latitude) * cos(longitude)
//
//  UV coordinates:
//      u = longitude / (2π)        ∈ [0, 1]
//      v = (latitude + π/2) / π    ∈ [0, 1]  (0 = south pole, 1 = north pole)
//
//  The normal of each vertex is simply its normalized position (outward-facing
//  for a unit sphere centered at the origin).
// ─────────────────────────────────────────────────────────────────────────────

class SphereGenerator {
public:
    // latitudeDivisions:  number of horizontal rings (more = smoother)
    // longitudeDivisions: number of vertical slices (more = smoother)
    // radius:             sphere radius in world units
    //
    // Recommended minimum: latitudeDivisions=32, longitudeDivisions=32
    static Mesh generate(int latitudeDivisions, int longitudeDivisions, float radius = 1.0f);
};
