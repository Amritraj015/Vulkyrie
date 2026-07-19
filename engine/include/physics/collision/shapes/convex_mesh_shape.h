#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/convex_polyhedron_shape.h"
#include "physics/types/convex_mesh.h"

namespace Vulkyrie {

    class ConvexMeshShape : public ConvexPolyhedronShape {
    public:
        explicit ConvexMeshShape(ConvexMesh *convexMesh, const glm::vec3 &scale = glm::vec3(1.0f));

        VE_DELETE_MOVE_AND_COPY(ConvexMeshShape);

        virtual ~ConvexMeshShape() override = default;

        VE_INLINE void SetScale(const glm::vec3 &scale) {
            _scale = scale;

            // Recompute the scaled face normals.
            computeScaledFaceNormals();

            // Notify collider shape change.
            NotifyCollidersOfShapeChange();
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetScale() const {
            return _scale;
        }

        [[nodiscard]] VE_INLINE size_t GetFacesCount() const override {
            return _convexMesh->GetHalfEdgeMesh().GetFaceCount();
        }

        [[nodiscard]] VE_INLINE size_t GetVerticesCount() const override {
            return _convexMesh->GetHalfEdgeMesh().GetVertexCount();
        }

        [[nodiscard]] VE_INLINE size_t GetHalfEdgesCount() const override {
            return _convexMesh->GetHalfEdgeMesh().GetHalfEdgeCount();
        }

        [[nodiscard]] VE_INLINE glm::vec3 GetVertexPosition(size_t vertexIndex) const override {
            VASSERT(vertexIndex < GetVerticesCount(), "Invalid vertex index.");

            return _convexMesh->GetVertex(vertexIndex) * _scale;
        }

        [[nodiscard]] VE_INLINE glm::vec3 GetFaceNormal(size_t faceIndex) const override {
            VASSERT(faceIndex < GetFacesCount(), "Invalid face index.");

            return _scaledFaceNormals[faceIndex];
        }

        [[nodiscard]] VE_INLINE glm::vec3 GetCentroid() const override {
            return _convexMesh->GetCentroid() * _scale;
        }

        [[nodiscard]] VE_INLINE const HalfEdgeMesh::Face &GetFace(size_t faceIndex) const override {
            VASSERT(faceIndex < GetFacesCount(), "Invalid face index.");

            return _convexMesh->GetHalfEdgeMesh().GetFace(faceIndex);
        }

        [[nodiscard]] VE_INLINE const HalfEdgeMesh::Edge &GetHalfEdge(size_t edgeIndex) const override {
            VASSERT(edgeIndex < GetHalfEdgesCount(), "Invalid half edge index.");

            return _convexMesh->GetHalfEdgeMesh().GetHalfEdge(edgeIndex);
        }

        [[nodiscard]] VE_INLINE const std::vector<HalfEdgeMesh::Edge> &GetHalfEdges() const override {
            return _convexMesh->GetHalfEdgeMesh().GetHalfEdges();
        }

        [[nodiscard]] VE_INLINE const HalfEdgeMesh::Vertex &GetVertex(size_t vertexIndex) const override {
            VASSERT(vertexIndex < GetVerticesCount(), "Invalid vertex index.");

            return _convexMesh->GetHalfEdgeMesh().GetVertex(vertexIndex);
        }

        [[nodiscard]] VE_INLINE AABB GetLocalAABB() const override {
            AABB aabb = _convexMesh->GetBounds();
            aabb.Scale(_scale);

            return aabb;
        }

        [[nodiscard]] VE_INLINE glm::vec3 GetLocalInertiaTensor(f32 mass) const override {
            return _convexMesh->GetLocalInertiaTensor(mass, _scale);
        }

        [[nodiscard]] VE_INLINE f32 GetVolume() const override {
            return _convexMesh->GetVolume() * _scale.x * _scale.y * _scale.z;
        }

        glm::vec3 GetLocalSupportPointWithoutMargin(const glm::vec3 &direction) const override;
        bool ContainsPoint(const glm::vec3 &point) const override;

    private:
        ConvexMesh *_convexMesh;
        glm::vec3 _scale;
        std::vector<glm::vec3> _scaledFaceNormals;

        void computeScaledFaceNormals();
    };

} // namespace Vulkyrie
