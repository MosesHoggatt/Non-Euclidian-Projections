#pragma once
#include "../math/Vector2.h"
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  geometry/GridGenerator.h
//
//  Generates a flat 2D grid of line segments in normalized [-1, 1]² space.
//
//  The grid is the INPUT to a projection. Each vertex in flat [-1, 1]² space
//  is passed through a Projection::mapFlatToSphere() to become a 3D point on
//  the target surface. The result is a visualization of how each projection
//  "wraps" a regular 2D grid onto the sphere.
//
//  Output format:
//    std::vector<Vector2> where every 2 consecutive entries are the endpoints
//    of one line segment (suitable for GL_LINES). Each logical grid line is
//    split into `segmentsPerLine` micro-segments so that curved projections
//    (stereographic, gnomonic) produce smooth arcs rather than jagged polygons.
//
//  Example: 12 lines, 64 segments per line:
//    - 12 horizontal + 12 vertical = 24 logical lines
//    - 24 × 64 segments × 2 vertices = 3072 Vector2 entries
//    - After projection: 3072 × 3 floats = 36KB GPU buffer — trivially fast
// ─────────────────────────────────────────────────────────────────────────────

class GridGenerator {
public:
    // lineCount:       number of horizontal lines + number of vertical lines.
    //                  They are evenly spaced within [-1, 1], symmetric about 0.
    //
    // segmentsPerLine: each logical line is subdivided into this many segments.
    //                  Use at least 32 for curved projections to look smooth.
    //                  64 is a good default.
    static std::vector<Vector2> generateGrid(int lineCount, int segmentsPerLine);
};
