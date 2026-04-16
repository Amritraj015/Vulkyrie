#pragma once

#include "physics/collision/shapes/convex_polyhedron_shape.h"

namespace Vulkyrie {

    class BoxShape final : public ConvexPolyhedronShape {
        public:
            /** @brief Construct a box shape with the specified half extents and margin.
             * @param halfExtents The half extents of the box shape as a glm::vec3, where each component represents half the size of the box along that axis.
             * All components must be positive.
             * @param margin The margin to be applied to the box shape for collision detection purposes. This is an optional parameter that defaults to 0.0f if
             * not provided. A positive margin can help improve collision detection stability by providing a small buffer around the shape.
             */
            BoxShape(const glm::vec3 &halfExtents, f32 margin = 0.0f);

            /** @brief Destructor for the BoxShape class. */
            ~BoxShape() override = default;

            // Delete the copy constructor and the copy assignment operator.
            BoxShape(const BoxShape &) = delete;
            BoxShape &operator=(const BoxShape &) = delete;

            // Delete the move constructor and the move assignment operator.
            BoxShape(BoxShape &&) = delete;
            BoxShape &operator=(BoxShape &&) = delete;

            /** @brief Get the half extents of the box shape.
             * @return The half extents of the box shape as a glm::vec3, where each component represents half the size of the box along that axis.
             */
            [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetHalfExtents() const {
                return _halfExtents;
            }

            /** @brief Set the half extents of the box shape.
             * @param halfExtents The new half extents of the box shape as a glm::vec3, where each component represents half the size of the box along that
             * axis. All components must be positive.
             */
            void SetHalfExtents(const glm::vec3 &halfExtents) {
                _halfExtents = halfExtents;
            }

            /** @brief Get the number of faces of the box shape.
             * @return The number of faces of the box shape, which is always 6 for a box.
             */
            [[nodiscard]] VE_FORCE_INLINE constexpr u32 GetFacesCount() const override {
                return 6;
            }

            /** @brief Get the number of vertices of the box shape.
             * @return The number of vertices of the box shape, which is always 8 for a box.
             */
            [[nodiscard]] VE_FORCE_INLINE constexpr u32 GetVerticesCount() const override {
                return 8;
            }

            /** @brief Get the number of half edges of the box shape.
             * @return The number of half edges of the box shape, which is always 24 for a box (each of the 12 edges has 2 half edges).
             */
            [[nodiscard]] VE_FORCE_INLINE constexpr u32 GetHafEdgesCount() const override {
                return 24;
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetVertexPosition(u32 vertexIndex) const override {
                VASSERT(vertexIndex < GetVerticesCount(), "Vertex index out of bounds for box shape.");

                switch (vertexIndex) {
                    case 0:
                        return glm::vec3(-_halfExtents.x, -_halfExtents.y, _halfExtents.z);
                    case 1:
                        return glm::vec3(_halfExtents.x, -_halfExtents.y, _halfExtents.z);
                    case 2:
                        return glm::vec3(_halfExtents.x, _halfExtents.y, _halfExtents.z);
                    case 3:
                        return glm::vec3(-_halfExtents.x, _halfExtents.y, _halfExtents.z);
                    case 4:
                        return glm::vec3(-_halfExtents.x, -_halfExtents.y, -_halfExtents.z);
                    case 5:
                        return glm::vec3(_halfExtents.x, -_halfExtents.y, -_halfExtents.z);
                    case 6:
                        return glm::vec3(_halfExtents.x, _halfExtents.y, -_halfExtents.z);
                    case 7:
                        return glm::vec3(-_halfExtents.x, _halfExtents.y, -_halfExtents.z);
                }

                VASSERT(false, "Invalid vertex index for box shape.");

                return glm::vec3(0.0f);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetFaceNormal(u32 faceIndex) const override {
                VASSERT(faceIndex < GetFacesCount(), "Face index out of bounds for box shape.");

                switch (faceIndex) {
                    case 0:
                        return glm::vec3(0.0f, 0.0f, 1.0f);
                    case 1:
                        return glm::vec3(1.0f, 0.0f, 0.0f);
                    case 2:
                        return glm::vec3(0.0f, 0.0f, -1.0f);
                    case 3:
                        return glm::vec3(-1.0f, 0.0f, 0.0f);
                    case 4:
                        return glm::vec3(0.0f, -1.0f, 0.0f);
                    case 5:
                        return glm::vec3(0.0f, 1.0f, 0.0f);
                }

                VASSERT(false, "Invalid face index for box shape.");

                return glm::vec3(0.0f);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetCentroid() const override {
                return glm::vec3(0.0f);
            }

            [[nodiscard]] VE_FORCE_INLINE AABB GetLocalAABB() const override {
                return AABB(-_halfExtents, _halfExtents);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLocalInertiaTensor(f32 mass) const override {
                const f32 factor = (f32(1.0) / f32(3.0)) * mass;
                const f32 xSquare = _halfExtents.x * _halfExtents.x;
                const f32 ySquare = _halfExtents.y * _halfExtents.y;
                const f32 zSquare = _halfExtents.z * _halfExtents.z;

                return glm::vec3(factor * (ySquare + zSquare), factor * (xSquare + zSquare), factor * (xSquare + ySquare));
            }

            [[nodiscard]] VE_FORCE_INLINE f32 GetVolume() const override {
                return 8.0f * _halfExtents.x * _halfExtents.y * _halfExtents.z;
            }

            [[nodiscard]] VE_FORCE_INLINE bool ContainsPoint(const glm::vec3 &point) const override {
                return (point.x < _halfExtents.x && point.x > -_halfExtents.x && point.y < _halfExtents.y && point.y > -_halfExtents.y &&
                        point.z < _halfExtents.z && point.z > -_halfExtents.z);
            }

        private:
            glm::vec3 _halfExtents;
    };

} // namespace Vulkyrie
