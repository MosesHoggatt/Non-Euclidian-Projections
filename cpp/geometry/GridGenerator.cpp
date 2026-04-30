#include "GridGenerator.h"

// ─────────────────────────────────────────────────────────────────────────────
//  geometry/GridGenerator.cpp
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Vector2> GridGenerator::generateGrid(int lineCount, int segmentsPerLine) {
    std::vector<Vector2> flatPoints;

    // Each logical line has `segmentsPerLine` segments, each needing 2 vertices.
    // Total vertices = 2 directions × lineCount lines × segmentsPerLine × 2 vertices.
    flatPoints.reserve(2 * lineCount * segmentsPerLine * 2);

    // Spacing between lines: divide the [-1, 1] range into (lineCount + 1) gaps,
    // then also include lines at the boundaries ±1.
    // Lines sit at: -1, -1+spacing, ..., 0, ..., 1-spacing, 1
    const float lineSpacing   = 2.0f / static_cast<float>(lineCount + 1);
    const float segmentLength = 2.0f / static_cast<float>(segmentsPerLine);

    // ── Horizontal lines (constant Y) ─────────────────────────────────────────
    for (int lineIndex = 0; lineIndex <= lineCount + 1; ++lineIndex) {
        float constantY = -1.0f + static_cast<float>(lineIndex) * lineSpacing;

        for (int segIndex = 0; segIndex < segmentsPerLine; ++segIndex) {
            float x0 = -1.0f + static_cast<float>(segIndex)     * segmentLength;
            float x1 = -1.0f + static_cast<float>(segIndex + 1) * segmentLength;

            flatPoints.push_back({ x0, constantY });
            flatPoints.push_back({ x1, constantY });
        }
    }

    // ── Vertical lines (constant X) ───────────────────────────────────────────
    for (int lineIndex = 0; lineIndex <= lineCount + 1; ++lineIndex) {
        float constantX = -1.0f + static_cast<float>(lineIndex) * lineSpacing;

        for (int segIndex = 0; segIndex < segmentsPerLine; ++segIndex) {
            float y0 = -1.0f + static_cast<float>(segIndex)     * segmentLength;
            float y1 = -1.0f + static_cast<float>(segIndex + 1) * segmentLength;

            flatPoints.push_back({ constantX, y0 });
            flatPoints.push_back({ constantX, y1 });
        }
    }

    return flatPoints;
}
