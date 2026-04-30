#include "SphereGenerator.h"
#include "../math/MathConstants.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  geometry/SphereGenerator.cpp
//
//  Builds a UV sphere using nested loops over latitude and longitude angles.
//
//  Ring layout:  latitude index 0 = south pole, index latitudeDivisions = north pole
//  Slice layout: longitude index 0 = +Z axis, increases counter-clockwise viewed from above
// ─────────────────────────────────────────────────────────────────────────────

Mesh SphereGenerator::generate(int latitudeDivisions, int longitudeDivisions, float radius) {
    Mesh mesh;

    // ── Generate vertices ─────────────────────────────────────────────────────
    // We iterate over (latitudeDivisions + 1) rings and (longitudeDivisions + 1) columns.
    // The extra +1 on each axis creates seam vertices with UV u=1/v=1 that are
    // geometrically identical to u=0/v=0 but have different texture coordinates,
    // which prevents a texture seam artifact.

    for (int latIndex = 0; latIndex <= latitudeDivisions; ++latIndex) {
        // Normalized latitude parameter t ∈ [0, 1], from south pole to north pole
        float latitudeParameter = static_cast<float>(latIndex) / static_cast<float>(latitudeDivisions);

        // Latitude angle: maps [0,1] → [-π/2, π/2]
        float latitudeAngle = latitudeParameter * MathConstants::PI - MathConstants::HALF_PI;

        float cosLatitude = std::cos(latitudeAngle);
        float sinLatitude = std::sin(latitudeAngle);

        // UV v coordinate: 0 at south pole, 1 at north pole
        float uvV = latitudeParameter;

        for (int lonIndex = 0; lonIndex <= longitudeDivisions; ++lonIndex) {
            float longitudeParameter = static_cast<float>(lonIndex) / static_cast<float>(longitudeDivisions);

            // Longitude angle: maps [0,1] → [0, 2π]
            float longitudeAngle = longitudeParameter * MathConstants::TWO_PI;

            float cosLongitude = std::cos(longitudeAngle);
            float sinLongitude = std::sin(longitudeAngle);

            // ── Vertex position ───────────────────────────────────────────────
            // Standard spherical-to-Cartesian conversion.
            // The Y axis is the polar axis (north = +Y).
            float posX = radius * cosLatitude * sinLongitude;
            float posY = radius * sinLatitude;
            float posZ = radius * cosLatitude * cosLongitude;

            // ── Vertex normal ─────────────────────────────────────────────────
            // For a unit sphere centered at the origin, the outward normal
            // is simply the normalized position vector.
            float normalX = cosLatitude * sinLongitude;
            float normalY = sinLatitude;
            float normalZ = cosLatitude * cosLongitude;

            // ── UV coordinates ────────────────────────────────────────────────
            float uvU = longitudeParameter;

            // Interleave: [px, py, pz, nx, ny, nz, u, v]
            mesh.vertexData.insert(mesh.vertexData.end(), {
                posX, posY, posZ,
                normalX, normalY, normalZ,
                uvU, uvV
            });
        }
    }

    // ── Generate triangle indices ─────────────────────────────────────────────
    // For each quad formed by adjacent ring/slice intersections, emit two triangles.
    //
    // Quad corners (ring i, slice j):
    //   topLeft     = (i+1) * (longitudeDivisions+1) + j
    //   topRight    = topLeft + 1
    //   bottomLeft  = i     * (longitudeDivisions+1) + j
    //   bottomRight = bottomLeft + 1

    int verticesPerRow = longitudeDivisions + 1;

    for (int latIndex = 0; latIndex < latitudeDivisions; ++latIndex) {
        for (int lonIndex = 0; lonIndex < longitudeDivisions; ++lonIndex) {
            unsigned int bottomLeft  = static_cast<unsigned int>(latIndex       * verticesPerRow + lonIndex);
            unsigned int bottomRight = static_cast<unsigned int>(latIndex       * verticesPerRow + lonIndex + 1);
            unsigned int topLeft     = static_cast<unsigned int>((latIndex + 1) * verticesPerRow + lonIndex);
            unsigned int topRight    = static_cast<unsigned int>((latIndex + 1) * verticesPerRow + lonIndex + 1);

            // Triangle 1: bottom-left → bottom-right → top-right
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
            mesh.indices.push_back(topRight);

            // Triangle 2: bottom-left → top-right → top-left
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);
            mesh.indices.push_back(topLeft);
        }
    }

    return mesh;
}
