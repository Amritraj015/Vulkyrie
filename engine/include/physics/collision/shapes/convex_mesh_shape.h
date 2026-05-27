#pragma once

#include "physics/collision/shapes/convex_polyhedron_shape.h"
#include "physics/types/convex_mesh.h"

namespace Vulkyrie {

    // class ConvexMeshShape : public ConvexPolyhedronShape {
    // public:
    //     ConvexMeshShape(ConvexMesh *convexMesh, const glm::vec3 &scale = glm::vec3(1.0f));
    //
    //     ConvexMeshShape(const ConvexMeshShape &) = delete;
    //     ConvexMeshShape &operator=(const ConvexMeshShape &) = delete;
    //
    //     ConvexMeshShape(ConvexMeshShape &&) = delete;
    //     ConvexMeshShape &operator=(ConvexMeshShape &&) = delete;
    //
    //     ~ConvexMeshShape() override = default;
    //
    //     VE_INLINE void SetScale(const glm::vec3 &scale) {
    //         _scale = scale;
    //
    //         //  Recompute the scaled face normals
    //         computeScaledFacesNormals();
    //
    //         NotifyCollidersOfShapeChange();
    //     }
    //
    //     [[nodiscard]] VE_INLINE const glm::vec3 &GetScale() const {
    //         return _scale;
    //     }
    //
    //     u32 GetFacesCount() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetFacesCount` is not implemented.");
    //     }
    //
    //     u32 GetVerticesCount() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetVerticesCount` is not implemented.");
    //     }
    //
    //     u32 GetHalfEdgesCount() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetHalfEdgesCount` is not implemented.");
    //     }
    //
    //     glm::vec3 GetVertexPosition(u32 vertexIndex) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetVertexPosition` is not implemented.");
    //     }
    //
    //     glm::vec3 GetFaceNormal(u32 faceIndex) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetFaceNormal` is not implemented.");
    //     }
    //
    //     glm::vec3 GetCentroid() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetCentroid` is not implemented.");
    //     }
    //
    //     const HalfEdgeMesh::Face &GetFace(size_t faceIndex) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetFace` is not implemented.");
    //     }
    //
    //     const HalfEdgeMesh::Edge &GetHalfEdge(size_t edgeIndex) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetHalfEdge` is not implemented.");
    //     }
    //
    //     const HalfEdgeMesh::Vertex &GetVertex(size_t vertexIndex) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetVertex` is not implemented.");
    //     }
    //
    //     glm::vec3 GetLocalSupportPointWithoutMargin(const glm::vec3 &direction) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetLocalSupportPointWithoutMargin` is not implemented.");
    //     }
    //
    //     bool IsConvex() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `IsConvex` is not implemented.");
    //     }
    //
    //     bool IsPolyhedral() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `IsPolyhedral` is not implemented.");
    //     }
    //
    //     AABB GetLocalAABB() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetLocalAABB` is not implemented.");
    //     }
    //
    //     glm::vec3 GetLocalInertiaTensor(f32 mass) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetLocalInertiaTensor` is not implemented.");
    //     }
    //
    //     f32 GetVolume() const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `GetVolume` is not implemented.");
    //     }
    //
    //     bool ContainsPoint(const glm::vec3 &point) const override {
    //         // TODO: Implement this pure virtual method.
    //         static_assert(false, "Method `ContainsPoint` is not implemented.");
    //     }
    //
    // private:
    //     ConvexMesh *_convexMesh;
    //     glm::vec3 _scale;
    //     std::vector<glm::vec3> _scaledFaceNormals;
    //
    //     void computeScaledFacesNormals();
    // };

} // namespace Vulkyrie
