#pragma once
#include <vector>
#include "../math/Vector3.h"
#include "../math/Vector2.h"

// ─────────────────────────────────────────────────────────────────────────────
//  geometry/Mesh.h
//
//  A CPU-side representation of a 3D mesh.
//  Stores interleaved vertex data in the format:
//
//      [ px, py, pz,  nx, ny, nz,  u, v ]  ← 8 floats per vertex
//       position(3)   normal(3)    uv(2)
//
//  This interleaved layout matches the vertex attribute setup in main.cpp:
//      location 0: position  (offset=0,  stride=32)
//      location 1: normal    (offset=12, stride=32)
//      location 2: uv        (offset=24, stride=32)
//
//  Indices reference into the vertex array to form triangles (3 indices = 1 triangle).
// ─────────────────────────────────────────────────────────────────────────────

struct Mesh {
    // Interleaved vertex data: [px, py, pz, nx, ny, nz, u, v, ...]
    std::vector<float>        vertexData;

    // Triangle index buffer: every 3 entries form one triangle
    std::vector<unsigned int> indices;

    // Convenience: total number of vertices
    int vertexCount() const {
        return static_cast<int>(vertexData.size()) / FLOATS_PER_VERTEX;
    }

    // Number of triangles
    int triangleCount() const {
        return static_cast<int>(indices.size()) / 3;
    }

    // Data layout constants — used in VertexAttributeDescriptor setup
    static constexpr int FLOATS_PER_VERTEX = 8;             // px py pz nx ny nz u v
    static constexpr int STRIDE_IN_BYTES   = 8 * sizeof(float);
    static constexpr int POSITION_OFFSET   = 0;
    static constexpr int NORMAL_OFFSET     = 3 * sizeof(float);
    static constexpr int UV_OFFSET         = 6 * sizeof(float);
};
