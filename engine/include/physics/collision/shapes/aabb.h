#pragma once

#include "vlkypch.h"
#include "core/asserts.h"

namespace Vulkyrie {

    /** @brief Represents an Axis-Aligned Bounding Box (AABB) defined by its minimum and maximum coordinates in 3D "world" space. The edges of the box are
     * aligned with the world axes, meaning that the box is not rotated and its faces are parallel to the coordinate planes. The AABB is used in collision
     * detection and spatial partitioning algorithms due to its simplicity and efficiency. */
    class AABB final {
    public:
        /** @brief Constructs an AABB from the given minimum and maximum corner coordinates.
         * @param minCoordinates The minimum corner of the box (smallest x, y, z values).
         * @param maxCoordinates The maximum corner of the box (largest x, y, z values). */
        AABB(const glm::vec3 &minCoordinates, const glm::vec3 &maxCoordinates);

        /** @brief Default destructor for AABB. */
        ~AABB() = default;

        /** @brief Returns the minimum corner of the AABB. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetMin() const {
            return _minCoordinates;
        }

        /** @brief Sets the minimum corner of the AABB.
         * @param min The new minimum corner coordinates. */
        VE_INLINE void SetMin(const glm::vec3 &min) {
            VASSERT(min.x <= _maxCoordinates.x && min.y <= _maxCoordinates.y && min.z <= _maxCoordinates.z, "New min must not exceed current max.");

            _minCoordinates = min;
        }

        /** @brief Returns the maximum corner of the AABB. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetMax() const {
            return _maxCoordinates;
        }

        /** @brief Sets the maximum corner of the AABB.
         * @param max The new maximum corner coordinates. */
        VE_INLINE void SetMax(const glm::vec3 &max) {
            VASSERT(max.x >= _minCoordinates.x && max.y >= _minCoordinates.y && max.z >= _minCoordinates.z, "New max must not be less than current min.");

            _maxCoordinates = max;
        }

        /** @brief Sets both the minimum and maximum corners of the AABB simultaneously. Use this instead of calling SetMin and SetMax separately when
         * both bounds need to change at the same time, as those methods validate against the current (stale) opposite bound and would fire a false
         * assertion.
         * @param min The new minimum corner coordinates.
         * @param max The new maximum corner coordinates. Must be component-wise >= min. */
        VE_INLINE void SetMinMax(const glm::vec3 &min, const glm::vec3 &max) {
            VASSERT(min.x <= max.x && min.y <= max.y && min.z <= max.z, "New min must not exceed new max.");

            _minCoordinates = min;
            _maxCoordinates = max;
        }

        /** @brief Returns the extents of the AABB, which are the lengths of the box along each axis. This is computed as the difference between the max and
         * min coordinates. */
        [[nodiscard]] VE_INLINE glm::vec3 GetExtents() const {
            return _maxCoordinates - _minCoordinates;
        }

        /** @brief Returns the center point of the AABB. */
        [[nodiscard]] VE_INLINE glm::vec3 GetCenter() const {
            return (_minCoordinates + _maxCoordinates) * 0.5f;
        }

        /** @brief Expands the AABB if necessary so that it contains the given point.
         * @param point The world-space point to include within the AABB. */
        VE_INLINE void Encapsulate(const glm::vec3 &point) {
            _minCoordinates = glm::min(_minCoordinates, point);
            _maxCoordinates = glm::max(_maxCoordinates, point);
        }

        /** @brief Grows the AABB uniformly by subtracting the inflation from the min corner and adding it to the max corner.
         * @param inflation The amount to expand along each axis. */
        VE_INLINE void Inflate(const glm::vec3 &inflation) {
            _minCoordinates -= inflation;
            _maxCoordinates += inflation;

            VASSERT(_minCoordinates.x <= _maxCoordinates.x && _minCoordinates.y <= _maxCoordinates.y && _minCoordinates.z <= _maxCoordinates.z,
                    "Inflation resulted in min exceeding max.");
        }

        /** @brief Scales the AABB relative to the world origin by multiplying both corners by the given scale factors.
         * @param scale The scale factors for each axis. Must be positive. */
        VE_INLINE void Scale(const glm::vec3 &scale) {
            VASSERT(scale.x > 0 && scale.y > 0 && scale.z > 0, "Scale factors must be positive.");

            _minCoordinates = _minCoordinates * scale;
            _maxCoordinates = _maxCoordinates * scale;
        }

        /** @brief Tests whether this AABB overlaps or touches another AABB using the separating axis theorem.
         * @param other The other AABB to test against.
         * @returns True if the two AABBs overlap or touch, false otherwise. */
        [[nodiscard]] VE_INLINE bool CollidesWith(const AABB &other) const {
            if (_maxCoordinates.x < other._minCoordinates.x || other._maxCoordinates.x < _minCoordinates.x) return false;
            if (_maxCoordinates.y < other._minCoordinates.y || other._maxCoordinates.y < _minCoordinates.y) return false;
            if (_maxCoordinates.z < other._minCoordinates.z || other._maxCoordinates.z < _minCoordinates.z) return false;

            return true;
        }

        /** @brief Computes the volume of the AABB.
         * @returns The volume of the box. */
        [[nodiscard]] VE_INLINE f32 GetVolume() const {
            glm::vec3 extents = _maxCoordinates - _minCoordinates;
            return extents.x * extents.y * extents.z;
        }

        /** @brief Tests whether a given point is inside or on the surface of this AABB, with an optional epsilon tolerance to account for floating-point
         * precision issues.
         * @param point The world-space point to test for containment.
         * @param epsilon A small tolerance value to allow points that are very close to the surface to be considered inside. Defaults to the smallest
         * representable positive float. This is useful to avoid false negatives due to floating-point inaccuracies when a point is very close to the
         * boundary of the AABB.
         * @returns True if the point is inside or on the surface of the AABB (within the epsilon tolerance), false otherwise. */
        [[nodiscard]] VE_INLINE bool Contains(const glm::vec3 &point, f32 epsilon = std::numeric_limits<f32>::epsilon()) const {
            return (point.x >= _minCoordinates.x - epsilon && point.x <= _maxCoordinates.x + epsilon) &&
                   (point.y >= _minCoordinates.y - epsilon && point.y <= _maxCoordinates.y + epsilon) &&
                   (point.z >= _minCoordinates.z - epsilon && point.z <= _maxCoordinates.z + epsilon);
        }

        /** @brief Tests whether this AABB completely contains another AABB, meaning that the other box is entirely inside or on the surface of this box.
         * @param other The other AABB to test for containment.
         * @returns True if this AABB contains the other AABB, false otherwise. */
        [[nodiscard]] VE_INLINE bool Contains(const AABB &other) const {
            return (other._minCoordinates.x >= _minCoordinates.x && other._maxCoordinates.x <= _maxCoordinates.x) &&
                   (other._minCoordinates.y >= _minCoordinates.y && other._maxCoordinates.y <= _maxCoordinates.y) &&
                   (other._minCoordinates.z >= _minCoordinates.z && other._maxCoordinates.z <= _maxCoordinates.z);
        }

        /** @brief Expands this AABB to encompass the volume of another AABB. The resulting AABB will be the smallest box that contains both the original
         * and the other AABB.
         * @param other The other AABB to merge with. */
        VE_INLINE void MergeWithAABB(const AABB &other) {
            _minCoordinates = glm::min(_minCoordinates, other._minCoordinates);
            _maxCoordinates = glm::max(_maxCoordinates, other._maxCoordinates);
        }

        /** @brief Sets this AABB to the smallest AABB that contains both given AABBs. This replaces this AABB's current bounds entirely with the
         * union of the two input boxes. Unlike MergeWithAABB, this does not incorporate the current bounds of this AABB into the result.
         * @param first The first AABB to merge.
         * @param second The second AABB to merge. */
        VE_INLINE void MergeTwoAABBs(const AABB &first, const AABB &second) {
            _minCoordinates = glm::min(first._minCoordinates, second._minCoordinates);
            _maxCoordinates = glm::max(first._maxCoordinates, second._maxCoordinates);
        }

        /** @brief Returns a new AABB that is the smallest box containing both input AABBs. This is a static version of MergeTwoAABBs that does not modify
         * an existing AABB but instead creates and returns a new one representing the union of the two input boxes.
         * @param first The first AABB to merge.
         * @param second The second AABB to merge.
         * @returns A new AABB that contains both input AABBs. */
        [[nodiscard]] VE_INLINE static AABB With(const AABB &first, const AABB &second) {
            return AABB(glm::min(first._minCoordinates, second._minCoordinates), glm::max(first._maxCoordinates, second._maxCoordinates));
        }

    private:
        /** @brief The world-space coordinates of the minimum corner of the AABB, representing the smallest x, y, and z values of the box.
         * This point is diagonally opposite to the maximum corner and defines the extent of the box along each axis. */
        glm::vec3 _minCoordinates;

        /** @brief The world-space coordinates of the maximum corner of the AABB, representing the largest x, y, and z values of the box.
         * This point is diagonally opposite to the minimum corner and defines the extent of the box along each axis. */
        glm::vec3 _maxCoordinates;
    };

} // namespace Vulkyrie
