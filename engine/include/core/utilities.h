#pragma once

#include "vlkypch.h"
#include "core/constants.h"

namespace Vulkyrie {

    /** Reads the contents of a file at the given path and returns it as a string.
     * @param path The path to the file to read.
     * @returns The contents of the file as a string, or std::nullopt if an error occurred.
     */
    [[nodiscard]] std::optional<std::string> ReadTextFromFile(const std::filesystem::path &path);

    /** @brief Pairs two 32-bit unsigned integers into a single 64-bit unsigned integer using a specific formula.
     *
     * The function takes two 32-bit unsigned integers, `number1` and `number2`, and combines them into a single 64-bit unsigned integer.
     * The formula used is: `number1 * number1 + number1 + number2`. This ensures that the resulting 64-bit integer is unique for each pair of input numbers,
     * as long as `number1` is greater than or equal to `number2`.
     *
     * @param number1 The first 32-bit unsigned integer. Must be greater than or equal to `number2`.
     * @param number2 The second 32-bit unsigned integer.
     * @returns A 64-bit unsigned integer that uniquely represents the pair of input numbers.
     */
    [[nodiscard]] VE_INLINE u64 PairNumbers(u32 number1, u32 number2) {
        assert(number1 == std::max(number1, number2));

        u64 nb1 = number1;
        u64 nb2 = number2;
        return nb1 * nb1 + nb1 + nb2;
    }

    /** @brief Computes the closest point on a line segment defined by `lineStart` and `lineEnd` to a given `point`.
     *
     * The function calculates the projection of the point onto the line defined by the segment, then clamps this projection to ensure it lies within the
     * segment extents. If the line segment is degenerate (i.e., `lineStart` and `lineEnd` are the same point), the function returns `lineStart`.
     *
     * @param lineStart The starting point of the line segment.
     * @param lineEnd The ending point of the line segment.
     * @param point The point from which to find the closest point on the line segment.
     * @returns The closest point on the line segment to the given point.
     */
    [[nodiscard]] VE_INLINE glm::vec3 ComputeClosestPointOnLineSegment(const glm::vec3 &lineStart, const glm::vec3 &lineEnd, const glm::vec3 &point) {
        const glm::vec3 lineDirection = lineEnd - lineStart;
        const f32 lineLengthSquared = glm::length2(lineDirection);

        if (lineLengthSquared < VE_MACHINE_EPSILON) {
            // The line segment is degenerate (start and end are the same point), so return the start point.
            return lineStart;
        }

        // Project the point onto the line defined by the segment, then clamp to the segment extents.
        const f32 t = glm::dot(point - lineStart, lineDirection) / lineLengthSquared;
        const f32 clampedT = glm::clamp(t, 0.0f, 1.0f);

        return lineStart + clampedT * lineDirection;
    }

} // namespace Vulkyrie
