#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  math/MathConstants.h
//
//  Shared mathematical constants used across the projection and geometry code.
// ─────────────────────────────────────────────────────────────────────────────

namespace MathConstants {

    constexpr float PI        = 3.14159265358979323846f;
    constexpr float TWO_PI    = 2.0f * PI;
    constexpr float HALF_PI   = 0.5f * PI;

    // Converts degrees to radians.
    constexpr float toRadians(float degrees) { return degrees * (PI / 180.0f); }

    // Converts radians to degrees.
    constexpr float toDegrees(float radians) { return radians * (180.0f / PI); }

    // Clamps a value between a minimum and maximum.
    constexpr float clamp(float value, float minValue, float maxValue) {
        return value < minValue ? minValue : (value > maxValue ? maxValue : value);
    }

} // namespace MathConstants
