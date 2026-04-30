#pragma once
#include "Mesh.h"

// ─────────────────────────────────────────────────────────────────────────────
//  TorusGenerator.h
//
//  Generates a UV torus mesh in the same interleaved format as SphereGenerator.
//  Parameterized by two angles:
//    φ (phi)   – major angle: rotation around the Y axis (0 to 2π)
//    θ (theta) – minor angle: rotation around the tube cross-section (0 to 2π)
//
//  Position formula:
//    x = (R + r·cosθ)·cosφ
//    y = r·sinθ
//    z = (R + r·cosθ)·sinφ
//
//  where R = majorRadius (center → tube center), r = minorRadius (tube radius).
//
//  Outward-facing normal (perpendicular to tube surface):
//    nx = cosθ·cosφ,  ny = sinθ,  nz = cosθ·sinφ
//
//  Vertex layout: [px,py,pz, nx,ny,nz, u,v] — 8 floats/vertex (see Mesh.h)
// ─────────────────────────────────────────────────────────────────────────────

class TorusGenerator {
public:
    // Generate a torus mesh.
    //  circleDivisions – segments around the major ring (Y axis)
    //  tubeDivisions   – segments around the tube cross-section
    //  majorRadius     – distance from torus center to tube center (default 0.65)
    //  minorRadius     – radius of the tube itself (default 0.28)
    static Mesh generate(int circleDivisions, int tubeDivisions,
                         float majorRadius = 0.65f, float minorRadius = 0.28f);
};
