#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "physics/collision/broadphase/dynamic_aabb_tree.h"

namespace Vulkyrie {

    class TriangleMesh final {
    public:
        TriangleMesh();

        [[nodiscard]] VE_INLINE size_t GetTriangleCount() const {
            return _triangleIndices.size() / 3;
        }

        [[nodiscard]] VE_INLINE size_t GetVertexCount() const {
            return _vertices.size();
        }

        VE_INLINE void GetTriangleVerticesIndices(size_t triangleIndex, size_t (&outVerticesIndices)[3]) const {
            VASSERT(triangleIndex < GetTriangleCount(), "Triangle index out of bounds in triangle mesh.");

            const size_t index = triangleIndex * 3;
            outVerticesIndices[0] = _triangleIndices[index];
            outVerticesIndices[1] = _triangleIndices[index + 1];
            outVerticesIndices[2] = _triangleIndices[index + 2];
        }

        VE_INLINE void GetTriangleVertices(size_t triangleIndex, glm::vec3 (&outTriangleVertices)[3]) const {
            VASSERT(triangleIndex < GetTriangleCount(), "Triangle index out of bounds in triangle mesh.");

            const size_t index = triangleIndex * 3;
            outTriangleVertices[0] = _vertices[_triangleIndices[index]];
            outTriangleVertices[1] = _vertices[_triangleIndices[index + 1]];
            outTriangleVertices[2] = _vertices[_triangleIndices[index + 2]];
        }

        VE_INLINE void GetTriangleVertexNormals(size_t triangleIndex, glm::vec3 (&outTriangleVertexNormals)[3]) const {
            VASSERT(triangleIndex < GetTriangleCount(), "Triangle index out of bounds in triangle mesh.");

            const size_t index = triangleIndex * 3;
            outTriangleVertexNormals[0] = _vertexNormals[_triangleIndices[index]];
            outTriangleVertexNormals[1] = _vertexNormals[_triangleIndices[index + 1]];
            outTriangleVertexNormals[2] = _vertexNormals[_triangleIndices[index + 2]];
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetVertex(size_t vertexIndex) const {
            VASSERT(vertexIndex < _vertices.size(), "Vertex index out of bounds in triangle mesh.");

            return _vertices[vertexIndex];
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetVertexNormal(size_t vertexNormalIndex) const {
            VASSERT(vertexNormalIndex < _vertexNormals.size(), "Vertex normal index out of bounds in triangle mesh.");

            return _vertexNormals[vertexNormalIndex];
        }

        const AABB &GetBounds() const;

    private:
        std::vector<glm::vec3> _vertices;
        std::vector<glm::vec3> _vertexNormals;
        std::vector<size_t> _triangleIndices;
        DynamicAABBTree _aabbTree;
        f32 _epsilon;
    };

} // namespace Vulkyrie
