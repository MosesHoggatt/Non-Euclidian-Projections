#include "TorusGenerator.h"
#include "../math/MathConstants.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  TorusGenerator.cpp
//
//  Constructs an interleaved torus mesh suitable for Blinn-Phong shading.
//  The outer loop (phi) travels around the major ring; the inner loop (theta)
//  travels around the tube cross-section.
//
//  We generate (circleDivisions+1) × (tubeDivisions+1) vertices so that the
//  seam column/row can have duplicate positions with UV = 1.0, preventing
//  texture artifacts at the wrap-around boundary.
// ─────────────────────────────────────────────────────────────────────────────

Mesh TorusGenerator::generate(int circleDivisions, int tubeDivisions,
                               float majorRadius, float minorRadius) {
    Mesh mesh;

    // ── Vertex generation ─────────────────────────────────────────────────────
    for (int i = 0; i <= circleDivisions; ++i) {
        float phi    = MathConstants::TWO_PI * static_cast<float>(i) / static_cast<float>(circleDivisions);
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);
        float u      = static_cast<float>(i) / static_cast<float>(circleDivisions);

        for (int j = 0; j <= tubeDivisions; ++j) {
            float theta    = MathConstants::TWO_PI * static_cast<float>(j) / static_cast<float>(tubeDivisions);
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);
            float v        = static_cast<float>(j) / static_cast<float>(tubeDivisions);

            // ── Position ─────────────────────────────────────────────────────
            float px = (majorRadius + minorRadius * cosTheta) * cosPhi;
            float py = minorRadius * sinTheta;
            float pz = (majorRadius + minorRadius * cosTheta) * sinPhi;

            // ── Normal (outward from tube center) ─────────────────────────────
            // The tube center lies at (R·cosφ, 0, R·sinφ).
            // The normal simply points from that center toward the surface point.
            float nx = cosTheta * cosPhi;
            float ny = sinTheta;
            float nz = cosTheta * sinPhi;

            // Interleaved layout: position (3), normal (3), uv (2)
            mesh.vertexData.push_back(px);
            mesh.vertexData.push_back(py);
            mesh.vertexData.push_back(pz);
            mesh.vertexData.push_back(nx);
            mesh.vertexData.push_back(ny);
            mesh.vertexData.push_back(nz);
            mesh.vertexData.push_back(u);
            mesh.vertexData.push_back(v);
        }
    }

    // ── Index generation (two CCW triangles per quad) ─────────────────────────
    for (int i = 0; i < circleDivisions; ++i) {
        for (int j = 0; j < tubeDivisions; ++j) {
            unsigned int v00 = static_cast<unsigned int>(i       * (tubeDivisions + 1) + j);
            unsigned int v10 = static_cast<unsigned int>((i + 1) * (tubeDivisions + 1) + j);
            unsigned int v01 = static_cast<unsigned int>(i       * (tubeDivisions + 1) + j + 1);
            unsigned int v11 = static_cast<unsigned int>((i + 1) * (tubeDivisions + 1) + j + 1);

            // Triangle 1: bottom-left, bottom-right, top-right
            mesh.indices.push_back(v00);
            mesh.indices.push_back(v10);
            mesh.indices.push_back(v11);

            // Triangle 2: bottom-left, top-right, top-left
            mesh.indices.push_back(v00);
            mesh.indices.push_back(v11);
            mesh.indices.push_back(v01);
        }
    }

    return mesh;
}
