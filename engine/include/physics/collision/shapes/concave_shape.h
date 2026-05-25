#pragma once

#include "physics/collision/shapes/collision_shape.h"
#include "physics/collision/shapes/triangle_shape.h"

namespace Vulkyrie {

    class ConcaveShape : public CollisionShape {
    public:
        ConcaveShape(CollisionShapeName name, const glm::vec3 &scale);

        // Delete the copy constructor and operator.
        ConcaveShape(const ConcaveShape &) = delete;
        ConcaveShape &operator=(const ConcaveShape &) = delete;

        // Delete the move constructor and operator.
        ConcaveShape(ConcaveShape &&) = delete;
        ConcaveShape &operator=(ConcaveShape &&) = delete;

        // Default destructor.
        virtual ~ConcaveShape() override = default;

        [[nodiscard]] VE_INLINE const glm::vec3 &GetScale() const {
            return _scale;
        }

        VE_INLINE void SetScale(const glm::vec3 &scale) {
            _scale = scale;

            NotifyCollidersOfShapeChange();
        }

        [[nodiscard]] VE_INLINE TriangleRaycastSide GetRaycastTestType() const {
            return _triangleRaycastSide;
        }

        VE_INLINE void SetRaycastTestType(TriangleRaycastSide triangleRaycastSide) {
            _triangleRaycastSide = triangleRaycastSide;
        }

        [[nodiscard]] VE_INLINE virtual constexpr bool IsConvex() const override {
            return false;
        }

        [[nodiscard]] VE_INLINE virtual constexpr bool IsPolyhedral() const override {
            return true;
        }

        [[nodiscard]] VE_INLINE virtual glm::vec3 GetLocalInertiaTensor(f32 mass) const override {
            return glm::vec3(mass);
        }

        [[nodiscard]] VE_INLINE virtual constexpr bool ContainsPoint([[maybe_unused]] const glm::vec3 &point) const override {
            return false;
        }

        [[nodiscard]] VE_INLINE virtual f32 GetVolume() const override;

        virtual void ComputeOverlappingTriangles(const AABB &localAABB,
                                                 std::vector<glm::vec3> &triangleVertices,
                                                 std::vector<glm::vec3> &triangleVerticesNormals,
                                                 std::vector<u32> &shapeIds) const = 0;

    private:
        glm::vec3 _scale;
        TriangleRaycastSide _triangleRaycastSide;
    };
} // namespace Vulkyrie
