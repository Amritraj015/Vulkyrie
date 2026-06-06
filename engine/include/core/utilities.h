#pragma once

#include "vlkypch.h"
#include "core/constants.h"
#include "core/asserts.h"

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
        VASSERT(number1 == std::max(number1, number2), "The first number must be greater than the second one.");

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

    /** @brief Computes the squared distance from a point to an infinite line defined by two points.
     *
     * Uses the cross product formula: distSq = length2(PA × PB) / length2(AB).
     * This avoids a square root and is faster than the segment version.
     * If the line is degenerate (lineStart == lineEnd), returns the squared distance to lineStart.
     *
     * @param lineStart A point on the line.
     * @param lineEnd A second (distinct) point on the line.
     * @param point The point from which to measure.
     * @returns The squared distance from the point to the infinite line.
     */
    [[nodiscard]] VE_INLINE f32 ComputeDistanceSquaredPointToLine(const glm::vec3 &lineStart, const glm::vec3 &lineEnd, const glm::vec3 &point) {
        const glm::vec3 ab = lineEnd - lineStart;
        const f32 lineLengthSquared = glm::length2(ab);

        if (lineLengthSquared < VE_MACHINE_EPSILON) {
            return glm::length2(point - lineStart);
        }

        const glm::vec3 cross = glm::cross(point - lineStart, point - lineEnd);
        return glm::length2(cross) / lineLengthSquared;
    }

    /** @brief Computes the parametric intersection of a line segment with a plane.
     *
     * The plane is defined by its normal and scalar offset d, satisfying dot(planeNormal, P) = planeD.
     * The segment is parameterized as P(t) = segmentStart + t * (segmentEnd - segmentStart).
     * Substituting into the plane equation gives: t = (planeD - dot(n, A)) / dot(n, B - A).
     *
     * Returns t in (-inf, +inf) when the segment is not parallel to the plane. The intersection
     * lies on the segment when t is in [0, 1]. Returns -1.0f if the segment is parallel to the plane
     * (no intersection or the segment lies in the plane).
     *
     * @param segmentStart The start point of the segment (A).
     * @param segmentEnd The end point of the segment (B).
     * @param planeD The scalar offset of the plane: dot(planeNormal, P) = planeD.
     * @param planeNormal The normal of the plane (need not be unit length).
     * @returns The parametric value t of the intersection, or -1.0f if the segment is parallel to the plane.
     */
    [[nodiscard]] VE_INLINE f32 ComputePlaneSegmentIntersection(const glm::vec3 &segmentStart,
                                                                const glm::vec3 &segmentEnd,
                                                                const f32 planeD,
                                                                const glm::vec3 &planeNormal) {
        constexpr f32 parallelEpsilon = 0.0001f;
        f32 t = -1.0f;

        // dot(n, B - A): denominator of the intersection formula.
        // If near zero, the segment is parallel to the plane.
        const f32 nDotAB = glm::dot(planeNormal, segmentEnd - segmentStart);

        if (std::abs(nDotAB) > parallelEpsilon) {
            // t = (d - dot(n, A)) / dot(n, B - A)
            t = (planeD - glm::dot(planeNormal, segmentStart)) / nDotAB;
        }

        return t;
    }

    /** @brief Determines if two vectors are parallel by checking if the squared length of their cross product is below a small threshold.
     *
     * This function computes the cross product of the two input vectors and checks if its squared length is less than the square of a small epsilon value.
     * If the squared length of the cross product is very small, it indicates that the vectors are parallel (or nearly parallel) since the cross product of
     * parallel vectors is zero.
     *
     * @param vectorOne The first vector to compare.
     * @param vectorTwo The second vector to compare.
     * @returns True if the vectors are parallel (or nearly parallel), false otherwise.
     */
    [[nodiscard]] VE_INLINE bool AreParallelVectors(const glm::vec3 &vectorOne, const glm::vec3 &vectorTwo) {
        return glm::length2(glm::cross(vectorOne, vectorTwo)) < VE_MACHINE_EPSILON * VE_MACHINE_EPSILON;
    }

    /** @brief Determines if two vectors are orthogonal by checking if the absolute value of their dot product is below a small threshold.
     *
     * @param vectorOne The first vector to compare.
     * @param vectorTwo The second vector to compare.
     * @returns True if the vectors are orthogonal (or nearly orthogonal), false otherwise.
     */
    [[nodiscard]] VE_INLINE bool AreOrthogonalVectors(const glm::vec3 &vectorOne, const glm::vec3 &vectorTwo) {
        return std::abs(glm::dot(vectorOne, vectorTwo)) < 0.001f;
    }

    /** @brief Computes the closest points between two line segments.
     *
     * Uses the parametric approach from "Real-Time Collision Detection" (Ericson, §5.1.9).
     * Each segment is parameterized as P(s) = segmentOneStart + s*(segmentOneEnd - segmentOneStart) and
     * Q(t) = segmentTwoStart + t*(segmentTwoEnd - segmentTwoStart), with s, t clamped to [0, 1].
     * Degenerate segments (where start == end) are handled as points.
     *
     * @param segmentOneStart The start point of the first segment.
     * @param segmentOneEnd The end point of the first segment.
     * @param segmentTwoStart The start point of the second segment.
     * @param segmentTwoEnd The end point of the second segment.
     * @param closestPointSegmentOne Output: the closest point on segment 1.
     * @param closestPointSegmentTwo Output: the closest point on segment 2.
     */
    VE_INLINE void ComputeClosestPointBetweenTwoSegments(const glm::vec3 &segmentOneStart,
                                                         const glm::vec3 &segmentOneEnd,
                                                         const glm::vec3 &segmentTwoStart,
                                                         const glm::vec3 &segmentTwoEnd,
                                                         glm::vec3 &closestPointSegmentOne,
                                                         glm::vec3 &closestPointSegmentTwo) {
        const glm::vec3 seg1Direction = segmentOneEnd - segmentOneStart;  // Direction vector of segment 1.
        const glm::vec3 seg2Direction = segmentTwoEnd - segmentTwoStart;  // Direction vector of segment 2.
        const glm::vec3 originOffset = segmentOneStart - segmentTwoStart; // Vector from seg2 start to seg1 start.

        const f32 seg1LengthSquared = glm::length2(seg1Direction); // a: squared length of segment 1.
        const f32 seg2LengthSquared = glm::length2(seg2Direction); // e: squared length of segment 2.
        const f32 d2DotR = glm::dot(seg2Direction, originOffset);  // f: used for projecting onto segment 2.

        f32 s, t; // Parametric values on segment 1 and segment 2 respectively.

        // If both segments degenerate into points, return the two endpoints.
        if (seg1LengthSquared <= VE_MACHINE_EPSILON && seg2LengthSquared <= VE_MACHINE_EPSILON) {
            closestPointSegmentOne = segmentOneStart;
            closestPointSegmentTwo = segmentTwoStart;
            return;
        }

        if (seg1LengthSquared <= VE_MACHINE_EPSILON) {
            // Segment 1 is a point: fix s = 0 and find the closest point on segment 2.
            s = 0.0f;
            t = std::clamp(d2DotR / seg2LengthSquared, 0.0f, 1.0f);
        } else {
            const f32 d1DotR = glm::dot(seg1Direction, originOffset); // c: used for projecting onto segment 1.

            if (seg2LengthSquared <= VE_MACHINE_EPSILON) {
                // Segment 2 is a point: fix t = 0 and find the closest point on segment 1.
                t = 0.0f;
                s = std::clamp(-d1DotR / seg1LengthSquared, 0.0f, 1.0f);
            } else {
                // General case: neither segment is degenerate.
                const f32 d1DotD2 = glm::dot(seg1Direction, seg2Direction);                  // b: dot product of both directions.
                const f32 denom = seg1LengthSquared * seg2LengthSquared - d1DotD2 * d1DotD2; // a*e - b²

                if (denom != 0.0f) {
                    // Segments are not parallel: compute and clamp s to segment 1.
                    s = std::clamp((d1DotD2 * d2DotR - d1DotR * seg2LengthSquared) / denom, 0.0f, 1.0f);
                } else {
                    // Segments are parallel: s is arbitrary, pick the start of segment 1.
                    s = 0.0f;
                }

                // Compute t from the unconstrained closest point on segment 2 to P(s).
                t = (d1DotD2 * s + d2DotR) / seg2LengthSquared;

                // If t falls outside [0, 1], clamp it and recompute s against the clamped endpoint.
                if (t < 0.0f) {
                    t = 0.0f;
                    s = std::clamp(-d1DotR / seg1LengthSquared, 0.0f, 1.0f);
                } else if (t > 1.0f) {
                    t = 1.0f;
                    s = std::clamp((d1DotD2 - d1DotR) / seg1LengthSquared, 0.0f, 1.0f);
                }
            }
        }

        // Reconstruct the closest points from the parametric values.
        closestPointSegmentOne = segmentOneStart + seg1Direction * s;
        closestPointSegmentTwo = segmentTwoStart + seg2Direction * t;
    }

    /** @brief Clips a polygon against a single plane using the Sutherland-Hodgman algorithm.
     *
     * Iterates over each edge of the input polygon and retains vertices on the positive side of the plane
     * (where dot(vertex - planePoint, planeNormal) >= 0). Edges that cross the plane boundary contribute
     * the intersection point so the clipped polygon remains closed.
     *
     * @param polygonVertices The ordered list of vertices forming the input polygon.
     * @param planePoint Any point on the clipping plane.
     * @param planeNormal The outward normal of the clipping plane (need not be unit length).
     * @param outClippedPolygonVertices Output: the vertices of the clipped polygon. Must be empty on entry.
     */
    VE_INLINE void clipPolygonWithPlane(const std::vector<glm::vec3> &polygonVertices,
                                        const glm::vec3 &planePoint,
                                        const glm::vec3 &planeNormal,
                                        std::vector<glm::vec3> &outClippedPolygonVertices) {
        size_t nbInputVertices = polygonVertices.size();

        VASSERT(outClippedPolygonVertices.size() == 0, "outClippedPolygonVertices must be empty");

        size_t vStartIndex = nbInputVertices - 1;

        const f32 planeNormalDotPlanePoint = glm::dot(planeNormal, planePoint);

        f32 vStartDotN = glm::dot(polygonVertices[vStartIndex] - planePoint, planeNormal);

        // For each edge of the polygon
        for (size_t vEndIndex = 0; vEndIndex < nbInputVertices; vEndIndex++) {

            const glm::vec3 &vStart = polygonVertices[vStartIndex];
            const glm::vec3 &vEnd = polygonVertices[vEndIndex];

            const f32 vEndDotN = glm::dot(vEnd - planePoint, planeNormal);

            // If the second vertex is in front of the clipping plane
            if (vEndDotN >= 0.0f) {

                // If the first vertex is not in front of the clipping plane
                if (vStartDotN < 0.0f) {

                    // The second point we keep is the intersection between the segment v1, v2 and the clipping plane
                    const f32 t = ComputePlaneSegmentIntersection(vStart, vEnd, planeNormalDotPlanePoint, planeNormal);

                    if (t >= 0.0f && t <= 1.0f) {
                        outClippedPolygonVertices.push_back(vStart + t * (vEnd - vStart));
                    } else {
                        outClippedPolygonVertices.push_back(vEnd);
                    }
                }

                // Add the second vertex
                outClippedPolygonVertices.push_back(vEnd);
            } else { // If the second vertex is behind the clipping plane

                // If the first vertex is in front of the clipping plane
                if (vStartDotN >= 0.0f) {

                    // The first point we keep is the intersection between the segment v1, v2 and the clipping plane
                    const f32 t = ComputePlaneSegmentIntersection(vStart, vEnd, -planeNormalDotPlanePoint, -planeNormal);

                    if (t >= 0.0f && t <= 1.0f) {
                        outClippedPolygonVertices.push_back(vStart + t * (vEnd - vStart));
                    } else {
                        outClippedPolygonVertices.push_back(vStart);
                    }
                }
            }

            vStartIndex = vEndIndex;
            vStartDotN = vEndDotN;
        }
    }

    /** @brief Projects a point onto a plane defined by a unit normal and a point on the plane.
     *
     * Computes P' = P - dot(n, P - Q) * n, where n is the unit plane normal and Q is the plane point.
     * The result is the nearest point on the plane to the input point.
     *
     * @param point The point to project.
     * @param unitPlaneNormal The unit normal of the plane. Must be normalized.
     * @param planePoint Any point on the plane.
     * @returns The projection of the point onto the plane.
     */
    VE_INLINE glm::vec3 ProjectPointOntoPlane(const glm::vec3 &point, const glm::vec3 &unitPlaneNormal, const glm::vec3 &planePoint) {
        return point - glm::dot(unitPlaneNormal, point - planePoint) * unitPlaneNormal;
    }

    /** @brief Clips a line segment against an ordered list of planes, returning the surviving portion.
     *
     * Applies each clipping plane in sequence, keeping only the part of the segment on the positive side
     * (dot(vertex - planePoint, planeNormal) >= 0). Each step replaces the current segment with the
     * clipped result before applying the next plane. Returns an empty vector if the segment is fully
     * clipped away.
     *
     * @param segA The start point of the segment.
     * @param segB The end point of the segment.
     * @param planesPoints A point on each clipping plane. Must be the same size as planesNormals.
     * @param planesNormals The outward normal of each clipping plane. Must be the same size as planesPoints.
     * @returns The vertices of the clipped segment (0 or 2 points), or an empty vector if fully clipped.
     */
    VE_INLINE std::vector<glm::vec3> ClipSegmentWithPlanes(const glm::vec3 &segA,
                                                           const glm::vec3 &segB,
                                                           const std::vector<glm::vec3> &planesPoints,
                                                           const std::vector<glm::vec3> &planesNormals) {
        VASSERT(planesPoints.size() == planesNormals.size(), "planesPoints size must be equal to planesNormals.");

        std::vector<glm::vec3> inputVertices;
        std::vector<glm::vec3> outputVertices;
        inputVertices.reserve(2);
        outputVertices.reserve(2);

        inputVertices.push_back(segA);
        inputVertices.push_back(segB);

        // For each clipping plane
        for (size_t p = 0; p < planesPoints.size(); p++) {

            // If there is no more vertices, stop
            if (inputVertices.size() == 0) return inputVertices;

            VASSERT(inputVertices.size() == 2, "Size of inputVertices must be 2.");

            outputVertices.clear();

            glm::vec3 &v1 = inputVertices[0];
            glm::vec3 &v2 = inputVertices[1];

            const f32 v1DotN = glm::dot(v1 - planesPoints[p], planesNormals[p]);
            const f32 v2DotN = glm::dot(v2 - planesPoints[p], planesNormals[p]);

            // If the second vertex is in front of the clipping plane
            if (v2DotN >= 0.0f) {

                // If the first vertex is not in front of the clipping plane
                if (v1DotN < 0.0f) {

                    // The second point we keep is the intersection between the segment v1, v2 and the clipping plane
                    f32 t = ComputePlaneSegmentIntersection(v1, v2, glm::dot(planesNormals[p], planesPoints[p]), planesNormals[p]);

                    if (t >= 0.0f && t <= 1.0f) {
                        outputVertices.push_back(v1 + t * (v2 - v1));
                    } else {
                        outputVertices.push_back(v2);
                    }
                } else {
                    outputVertices.push_back(v1);
                }

                // Add the second vertex
                outputVertices.push_back(v2);
            } else { // If the second vertex is behind the clipping plane

                // If the first vertex is in front of the clipping plane
                if (v1DotN >= 0.0f) {

                    outputVertices.push_back(v1);

                    // The first point we keep is the intersection between the segment v1, v2 and the clipping plane
                    f32 t = ComputePlaneSegmentIntersection(v1, v2, glm::dot(-planesNormals[p], planesPoints[p]), -planesNormals[p]);

                    if (t >= 0.0f && t <= 1.0f) {
                        outputVertices.push_back(v1 + t * (v2 - v1));
                    }
                }
            }

            inputVertices = outputVertices;
        }

        return outputVertices;
    }

} // namespace Vulkyrie
