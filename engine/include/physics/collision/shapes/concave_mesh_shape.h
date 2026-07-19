#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "physics/collision/shapes/concave_shape.h"
#include "physics/types/triangle_mesh.h"

namespace Vulkyrie {

    class ConcaveMeshShape : public ConcaveShape {
    public:
        ConcaveMeshShape(TriangleMesh *triangleMesh, HalfEdgeMesh &triangleHalfEdgeMesh, const glm::vec3 &scaling = glm::vec3(1));

        VE_DELETE_MOVE_AND_COPY(ConcaveMeshShape);

        virtual ~ConcaveMeshShape() override = default;

        [[nodiscard]] VE_INLINE u32 ComputeTriangleShapeId(size_t triangleIndex) const {
            return static_cast<u32>(GetTriangleCount() + triangleIndex);
        }

        VE_INLINE virtual void SetScale(const glm::vec3 &scale) override {
            ConcaveShape::SetScale(scale);

            // Recompute scale vertex normals
            computeScaledVertexNormals();
        }

        [[nodiscard]] VE_INLINE const glm::vec3 GetVertex(size_t vertexIndex) const {
            VASSERT(vertexIndex < _triangleMesh->GetVertexCount(), "Invalid vertex index.");

            return _triangleMesh->GetVertex(vertexIndex) * _scale;
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetVertexNormal(size_t vertexIndex) const {
            VASSERT(vertexIndex < _triangleMesh->GetVertexCount(), "Invalid vertex index.");

            return _scaledVertexNormals[vertexIndex];
        }

        [[nodiscard]] VE_INLINE size_t GetVertexCount() const {
            return _triangleMesh->GetVertexCount();
        }

        [[nodiscard]] VE_INLINE size_t GetTriangleCount() const {
            return _triangleMesh->GetTriangleCount();
        }

        VE_INLINE void GetTriangleVerticesIndices(size_t triangleIndex, std::array<size_t, 3> &outVertexIndices) const {
            _triangleMesh->GetTriangleVerticesIndices(triangleIndex, outVertexIndices);
        }

        void GetTriangleVertices(size_t triangleIndex, std::array<glm::vec3, 3> &outVertices) const;
        void GetTriangleVerticesNormals(size_t triangleIndex, std::array<glm::vec3, 3> &outNormals) const;

        virtual AABB GetLocalAABB() const override;
        virtual void ComputeOverlappingTriangles(const AABB &localAABB,
                                                 std::vector<glm::vec3> &triangleVertices,
                                                 std::vector<glm::vec3> &triangleVerticesNormals,
                                                 std::vector<u32> &shapeIds) const override;

    private:
        TriangleMesh *_triangleMesh;
        [[maybe_unused]] HalfEdgeMesh &_triangleHalfEdgeMesh;
        std::vector<glm::vec3> _scaledVertexNormals;

        size_t getDynamicAABBTreeNodeData(u32 nodeID) const;
        void computeScaledVertexNormals();
    };

} // namespace Vulkyrie
