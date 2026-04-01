#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Physics {

    /** @brief Represents an Axis-Aligned Bounding Box (AABB) defined by its minimum and maximum coordinates in 3D "world" space. The edges of the box are
     * aligned with the world axes, meaning that the box is not rotated and its faces are parallel to the coordinate planes. The AABB is used in collision
     * detection and spatial partitioning algorithms due to its simplicity and efficiency. */
    class AABB {
        public:
            /** @brief Constructs an AABB from the given minimum and maximum corner coordinates.
             * @param minCoordinates The minimum corner of the box (smallest x, y, z values).
             * @param maxCoordinates The maximum corner of the box (largest x, y, z values). */
            AABB(const glm::vec3 &minCoordinates, const glm::vec3 &maxCoordinates)
                : _minCoordinates(minCoordinates)
                , _maxCoordinates(maxCoordinates) {
                assert(minCoordinates.x <= maxCoordinates.x && minCoordinates.y <= maxCoordinates.y && minCoordinates.z <= maxCoordinates.z &&
                       "Min coordinates must not exceed max coordinates.");
            }

            /** @brief Returns the center point of the AABB. */
            [[nodiscard]] glm::vec3 GetCenter() const {
                return (_minCoordinates + _maxCoordinates) * 0.5f;
            }

            /** @brief Expands the AABB if necessary so that it contains the given point.
             * @param point The world-space point to include within the AABB. */
            void Encapsulate(const glm::vec3 &point) {
                _minCoordinates = glm::min(_minCoordinates, point);
                _maxCoordinates = glm::max(_maxCoordinates, point);
            }

            /** @brief Grows the AABB uniformly by subtracting the inflation from the min corner and adding it to the max corner.
             * @param inflation The amount to expand along each axis. */
            void Inflate(const glm::vec3 &inflation) {
                _minCoordinates -= inflation;
                _maxCoordinates += inflation;

                assert(_minCoordinates.x <= _maxCoordinates.x && _minCoordinates.y <= _maxCoordinates.y && _minCoordinates.z <= _maxCoordinates.z &&
                       "Inflation resulted in min exceeding max.");
            }

            /** @brief Scales the AABB relative to the world origin by multiplying both corners by the given scale factors.
             * @param scale The scale factors for each axis. Must be positive. */
            void Scale(const glm::vec3 &scale) {
                assert(scale.x > 0 && scale.y > 0 && scale.z > 0 && "Scale factors must be positive.");

                _minCoordinates = _minCoordinates * scale;
                _maxCoordinates = _maxCoordinates * scale;
            }

            /** @brief Tests whether this AABB overlaps or touches another AABB using the separating axis theorem.
             * @param other The other AABB to test against.
             * @return True if the two AABBs overlap or touch, false otherwise. */
            [[nodiscard]] bool CollidesWith(const AABB &other) const {
                if (_maxCoordinates.x < other._minCoordinates.x || other._maxCoordinates.x < _minCoordinates.x) return false;
                if (_maxCoordinates.y < other._minCoordinates.y || other._maxCoordinates.y < _minCoordinates.y) return false;
                if (_maxCoordinates.z < other._minCoordinates.z || other._maxCoordinates.z < _minCoordinates.z) return false;

                return true;
            }

            /** @brief Computes the volume of the AABB.
             * @return The volume of the box. */
            [[nodiscard]] f32 GetVolume() const {
                glm::vec3 extents = _maxCoordinates - _minCoordinates;
                return extents.x * extents.y * extents.z;
            }

            /** @brief Tests whether the given point lies inside or on the surface of the AABB.
             * @param point The world-space point to test.
             * @return True if the point is inside or on the boundary of the AABB, false otherwise. */
            [[nodiscard]] bool ContainsPoint(const glm::vec3 &point) const {
                return (point.x >= _minCoordinates.x && point.x <= _maxCoordinates.x) && (point.y >= _minCoordinates.y && point.y <= _maxCoordinates.y) &&
                       (point.z >= _minCoordinates.z && point.z <= _maxCoordinates.z);
            }

            /** @brief Returns the minimum corner of the AABB. */
            [[nodiscard]] const glm::vec3 &GetMin() const {
                return _minCoordinates;
            }

            /** @brief Sets the minimum corner of the AABB.
             * @param min The new minimum corner coordinates. */
            void SetMin(const glm::vec3 &min) {
                assert(min.x <= _maxCoordinates.x && min.y <= _maxCoordinates.y && min.z <= _maxCoordinates.z && "New min must not exceed current max.");

                _minCoordinates = min;
            }

            /** @brief Returns the maximum corner of the AABB. */
            [[nodiscard]] const glm::vec3 &GetMax() const {
                return _maxCoordinates;
            }

            /** @brief Sets the maximum corner of the AABB.
             * @param max The new maximum corner coordinates. */
            void SetMax(const glm::vec3 &max) {
                assert(max.x >= _minCoordinates.x && max.y >= _minCoordinates.y && max.z >= _minCoordinates.z && "New max must not be less than current min.");

                _maxCoordinates = max;
            }

        private:
            /** @brief The world-space coordinates of the minimum corner of the AABB, representing the smallest x, y, and z values of the box.
             * This point is diagonally opposite to the maximum corner and defines the extent of the box along each axis. */
            glm::vec3 _minCoordinates;

            /** @brief The world-space coordinates of the maximum corner of the AABB, representing the largest x, y, and z values of the box.
             * This point is diagonally opposite to the minimum corner and defines the extent of the box along each axis. */
            glm::vec3 _maxCoordinates;
    };

} // namespace Vulkyrie::Physics
