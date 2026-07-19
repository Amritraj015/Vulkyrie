#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "physics/collision/broadphase/dynamic_aabb_tree.h"

namespace Vulkyrie {

    class TriangleMesh final {
    public:
        TriangleMesh();

        VE_DELETE_MOVE_AND_COPY(TriangleMesh);

        ~TriangleMesh() = default;

        [[nodiscard]] VE_INLINE size_t GetTriangleCount() const {
            return _triangleIndices.size() / 3;
        }

        [[nodiscard]] VE_INLINE size_t GetVertexCount() const {
            return _vertices.size();
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetVertex(size_t vertexIndex) const {
            VASSERT(vertexIndex < _vertices.size(), "Vertex index out of bounds in triangle mesh.");

            return _vertices[vertexIndex];
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetVertexNormal(size_t vertexNormalIndex) const {
            VASSERT(vertexNormalIndex < _vertexNormals.size(), "Vertex normal index out of bounds in triangle mesh.");

            return _vertexNormals[vertexNormalIndex];
        }

        [[nodiscard]] VE_INLINE size_t GetDynamicAABBTreeNodeData(u32 nodeID) const {
            return _aabbTree.GetNodeData(nodeID);
        }

        VE_INLINE void ReportAllShapesOverlappingWithAABB(const AABB &aabb, std::vector<u32> &overlappingNodes) {
            _aabbTree.QueryOverlaps(aabb, overlappingNodes);
        }

        [[nodiscard]] VE_INLINE const AABB &GetBounds() const {
            return _aabbTree.GetRootNodeAABB();
        }

        VE_INLINE void GetTriangleVerticesIndices(size_t triangleIndex, std::array<size_t, 3> &outVerticesIndices) const {
            VASSERT(triangleIndex < GetTriangleCount(), "Triangle index out of bounds in triangle mesh.");

            const size_t index = triangleIndex * 3;
            outVerticesIndices[0] = _triangleIndices[index];
            outVerticesIndices[1] = _triangleIndices[index + 1];
            outVerticesIndices[2] = _triangleIndices[index + 2];
        }

        VE_INLINE void GetTriangleVertices(size_t triangleIndex, std::array<glm::vec3, 3> &outTriangleVertices) const {
            VASSERT(triangleIndex < GetTriangleCount(), "Triangle index out of bounds in triangle mesh.");

            const size_t index = triangleIndex * 3;
            outTriangleVertices[0] = _vertices[_triangleIndices[index]];
            outTriangleVertices[1] = _vertices[_triangleIndices[index + 1]];
            outTriangleVertices[2] = _vertices[_triangleIndices[index + 2]];
        }

        VE_INLINE void GetTriangleVertexNormals(size_t triangleIndex, std::array<glm::vec3, 3> &outTriangleVertexNormals) const {
            VASSERT(triangleIndex < GetTriangleCount(), "Triangle index out of bounds in triangle mesh.");

            const size_t index = triangleIndex * 3;
            outTriangleVertexNormals[0] = _vertexNormals[_triangleIndices[index]];
            outTriangleVertexNormals[1] = _vertexNormals[_triangleIndices[index + 1]];
            outTriangleVertexNormals[2] = _vertexNormals[_triangleIndices[index + 2]];
        }

    private:
        DynamicAABBTree _aabbTree;
        std::vector<glm::vec3> _vertices;
        std::vector<glm::vec3> _vertexNormals;
        std::vector<size_t> _triangleIndices;
        [[maybe_unused]] f32 _epsilon;

        // bool copyVertices(const TriangleVertexArray &triangleVertexArray, std::vector<Message> &messages);
        // void computeVerticesNormals();
        // void computeEpsilon(const TriangleVertexArray &triangleVertexArray);
        // bool copyData(const TriangleVertexArray &triangleVertexArray, std::vector<Message> &errors);
        // void initBVHTree();
        // bool init(const TriangleVertexArray &triangleVertexArray, std::vector<Message> &messages);
        // void removeUnusedVertices(Array<bool> &areUsedVertices);
        // void raycast(const Ray &ray, DynamicAABBTreeRaycastCallback &callback) const;
    };

} // namespace Vulkyrie
