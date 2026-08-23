#include "physics/collision/shapes/concave_mesh_shape.h"

namespace Vulkyrie {

    ConcaveMeshShape::ConcaveMeshShape(TriangleMesh *triangleMesh, HalfEdgeMesh &triangleHalfEdgeMesh, const glm::vec3 &scaling)
        : ConcaveShape(CollisionShapeName::TriangleMesh, scaling)
        , _triangleMesh(triangleMesh)
        , _triangleHalfEdgeMesh(triangleHalfEdgeMesh)
        , _scaledVertexNormals(triangleMesh->GetVertexCount()) {

        _triangleRaycastSide = TriangleRaycastSide::Front;

        computeScaledVertexNormals();
    }

    void ConcaveMeshShape::GetTriangleVertices(size_t triangleIndex, std::array<glm::vec3, 3> &outVertices) const {
        _triangleMesh->GetTriangleVertices(triangleIndex, outVertices);

        outVertices[0] = outVertices[0] * _scale;
        outVertices[1] = outVertices[1] * _scale;
        outVertices[2] = outVertices[2] * _scale;
    }

    void ConcaveMeshShape::GetTriangleVerticesNormals(size_t triangleIndex, std::array<glm::vec3, 3> &outNormals) const {
        VASSERT(triangleIndex < _triangleMesh->GetTriangleCount(), "Invalid triangle index.");

        std::array<size_t, 3> vertexIndices;
        _triangleMesh->GetTriangleVerticesIndices(triangleIndex, vertexIndices);

        outNormals[0] = _scaledVertexNormals[vertexIndices[0]];
        outNormals[1] = _scaledVertexNormals[vertexIndices[1]];
        outNormals[2] = _scaledVertexNormals[vertexIndices[2]];
    }

    AABB ConcaveMeshShape::GetLocalAABB() const {
        AABB aabb = _triangleMesh->GetBounds();
        aabb.Scale(_scale);

        return aabb;
    }

    void ConcaveMeshShape::ComputeOverlappingTriangles(const AABB &localAABB,
                                                       std::vector<glm::vec3> &triangleVertices,
                                                       std::vector<glm::vec3> &triangleVerticesNormals,
                                                       std::vector<u32> &shapeIds) const {
        AABB aabb(localAABB);
        aabb.Scale(f32(1.0) / _scale);

        std::vector<u32> overlappingNodes;
        overlappingNodes.reserve(64);

        _triangleMesh->ReportAllShapesOverlappingWithAABB(aabb, overlappingNodes);

        const size_t OverlappingNodeCount = overlappingNodes.size();

        triangleVertices.reserve(OverlappingNodeCount * 3);
        triangleVertices.resize(OverlappingNodeCount * 3);

        triangleVerticesNormals.reserve(OverlappingNodeCount * 3);
        triangleVerticesNormals.resize(OverlappingNodeCount * 3);

        std::array<glm::vec3, 3> temp;

        for (size_t i = 0; i < OverlappingNodeCount; i++) {
            u32 nodeId = overlappingNodes[i];
            size_t data = _triangleMesh->GetDynamicAABBTreeNodeData(nodeId);

            GetTriangleVertices(data, temp);

            triangleVertices[i * 3] = temp[0];
            triangleVertices[i * 3 + 1] = temp[1];
            triangleVertices[i * 3 + 2] = temp[2];

            GetTriangleVerticesNormals(data, temp);

            triangleVerticesNormals[i * 3] = temp[0];
            triangleVerticesNormals[i * 3 + 1] = temp[1];
            triangleVerticesNormals[i * 3 + 2] = temp[2];

            shapeIds.push_back(ComputeTriangleShapeId(data));
        }
    }

    size_t ConcaveMeshShape::getDynamicAABBTreeNodeData(u32 nodeID) const {
        return _triangleMesh->GetDynamicAABBTreeNodeData(nodeID);
    }

    void ConcaveMeshShape::computeScaledVertexNormals() {
        _scaledVertexNormals.clear();

        for (size_t v = 0; v < _triangleMesh->GetVertexCount(); v++) {
            glm::vec3 normal = _triangleMesh->GetVertexNormal(v);
            normal = (f32(1.0) / _scale) * normal;

            const f32 normalLength = glm::length(normal);

            VASSERT(VE_K_MACHINE_EPSILON < normalLength, "Normal length be greater than VE_MACHINE_EPSILON.");

            normal /= normalLength;

            _scaledVertexNormals.push_back(normal);
        }
    }

} // namespace Vulkyrie
