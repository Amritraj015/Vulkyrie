#include "physics/collision/shapes/height_field_shape.h"

namespace Vulkyrie {

    HeightFieldShape::HeightFieldShape(HeightField *heightField, const glm::vec3 &scaling)
        : ConcaveShape(CollisionShapeName::HeightField, scaling)
        , _heightField(heightField) {
    }

    void HeightFieldShape::ComputeOverlappingTriangles(const AABB &localAABB,
                                                       std::vector<glm::vec3> &triangleVertices,
                                                       std::vector<glm::vec3> &triangleVerticesNormals,
                                                       std::vector<u32> &shapeIds) const {

        const glm::vec3 inverseScale = f32(1.0) / _scale;
        const AABB aabb(localAABB.GetMin() * inverseScale, localAABB.GetMax() * inverseScale);

        _heightField->ComputeOverlappingTriangles(aabb, triangleVertices, triangleVerticesNormals, shapeIds, _scale);
    }

    AABB HeightFieldShape::GetLocalAABB() const {
        AABB aabb = _heightField->GetBounds();
        aabb.Scale(_scale);

        return aabb;
    }

} // namespace Vulkyrie
