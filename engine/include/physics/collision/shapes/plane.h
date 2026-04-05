#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief Represents an infinite plane in 3D space defined by a normal vector and a distance from the origin along the normal vector. */
    class Plane final {
        public:
            /** @brief Constructs a plane with the given normal vector and distance from the origin.
             * @param normal The normal vector of the plane, which should be normalized to have a length of 1. The normal vector defines the orientation of the
             * plane.
             * @param planeOffset The distance from the origin to the plane along the normal vector. This value can be positive, negative, or zero,
             * depending on the plane's position relative to the origin.
             */
            explicit Plane(const glm::vec3 &normal, f32 planeOffset) noexcept
                : _normal(glm::normalize(normal))
                , _planeOffset(planeOffset) {
            }

            /** @brief Rotates the plane by applying the given rotation matrix to its normal vector.
             * @param rotationMatrix The 3x3 rotation matrix to apply to the plane's normal vector. This matrix should represent a valid rotation (i.e., it
             * should be orthogonal and have a determinant of 1).
             */
            void Rotate(const glm::mat3 &rotationMatrix) noexcept {
                _normal = rotationMatrix * _normal;
            }

            /** @brief Gets the signed distance from a point to the plane.
             * @param point The point for which to calculate the signed distance to the plane.
             * @return The signed distance from the point to the plane. A positive value indicates that the point is in front of the plane, a negative value
             * indicates that it is behind the plane, and zero indicates that it is on the plane.
             */
            [[nodiscard]] f32 GetSignedDistance(const glm::vec3 &point) const noexcept {
                return glm::dot(_normal, point) - _planeOffset;
            }

            /** @brief Gets the normal vector of the plane.
             * @return The normal vector of the plane, normalized to have a length of 1.
             */
            [[nodiscard]] const glm::vec3 &GetNormal() const noexcept {
                return _normal;
            }

        private:
            /** @brief The normal vector of the plane, normalized to have a length of 1. */
            glm::vec3 _normal;

            /** @brief The distance from the origin to the plane along the normal vector. */
            f32 _planeOffset;
    };

} // namespace Vulkyrie
