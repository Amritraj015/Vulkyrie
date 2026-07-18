#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/types/half_edge_mesh.h"

namespace Vulkyrie {

    class ConvexMesh final {
    public:
        ConvexMesh();

        [[nodiscard]] VE_INLINE size_t GetFacesCount() const {
            return _halfEdgeMesh.GetFaceCount();
        }

        [[nodiscard]] VE_INLINE size_t GetVerticesCount() const {
            return _vertices.size();
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetVertex(size_t vertexIndex) const {
            VASSERT(vertexIndex < _vertices.size(), "Vertex index out of bounds.");

            return _vertices[vertexIndex];
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetFaceNormal(size_t faceIndex) const {
            VASSERT(faceIndex < GetFacesCount(), "Face index out of bounds.");

            return _faceNormals[faceIndex];
        }

        [[nodiscard]] VE_INLINE const HalfEdgeMesh &GetHalfEdgeMesh() const {
            return _halfEdgeMesh;
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetCentroid() const {
            return _centroid;
        }

        [[nodiscard]] VE_INLINE const AABB &GetBounds() const {
            return _bounds;
        }

        [[nodiscard]] VE_INLINE f32 GetVolume() const {
            return _volume;
        }

        [[nodiscard]] VE_INLINE glm::vec3 GetLocalInertiaTensor(f32 mass, glm::vec3 scale) const {
            const f32 factor = (f32(1.0) / f32(3.0)) * mass;
            const glm::vec3 realExtent = f32(0.5) * scale * (_bounds.GetMax() - _bounds.GetMin());

            VASSERT(realExtent.x > 0 && realExtent.y > 0 && realExtent.z > 0, "Invalid mesh bounds for inertia tensor calculation. Extents must be positive.");

            const f32 xSquare = realExtent.x * realExtent.x;
            const f32 ySquare = realExtent.y * realExtent.y;
            const f32 zSquare = realExtent.z * realExtent.z;

            return glm::vec3(factor * (ySquare + zSquare), factor * (xSquare + zSquare), factor * (xSquare + ySquare));
        }

    private:
        HalfEdgeMesh _halfEdgeMesh;
        std::vector<glm::vec3> _vertices;
        std::vector<glm::vec3> _faceNormals;
        glm::vec3 _centroid;
        AABB _bounds;
        f32 _volume;

        // bool init(const PolygonVertexArray& polygonVertexArray, std::vector<Message>& errors);
        // bool copyVertices(const PolygonVertexArray& polygonVertexArray, std::vector<Message>& errors);
        // bool createHalfEdgeStructure(const PolygonVertexArray& polygonVertexArray, std::vector<Message>& errors);
        // bool computeFacesNormals(std::vector<Message>& errors);

        glm::vec3 computeFaceNormal(size_t faceIndex) const;

        void computeVolume();
    };

} // namespace Vulkyrie
