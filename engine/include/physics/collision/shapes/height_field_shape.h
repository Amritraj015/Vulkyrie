#pragma once

#include "physics/types/height_field.h"
#include "vlkypch.h"
#include "physics/collision/shapes/concave_shape.h"

namespace Vulkyrie {

    class HeightFieldShape final : public ConcaveShape {
    public:
        VE_DELETE_MOVE_AND_COPY(HeightFieldShape);

        [[nodiscard]] VE_INLINE HeightField *GetHeightField() const {
            return _heightField;
        }

        [[nodiscard]] VE_INLINE glm::vec3 GetVertexAt(size_t x, size_t y) const {
            return _heightField->GetVertexAt(x, y) * _scale;
        }

        void ComputeOverlappingTriangles(const AABB &localAABB,
                                         std::vector<glm::vec3> &triangleVertices,
                                         std::vector<glm::vec3> &triangleVerticesNormals,
                                         std::vector<u32> &shapeIds) const override;

        AABB GetLocalAABB() const override;

    private:
        HeightField *_heightField;
    };

} // namespace Vulkyrie
