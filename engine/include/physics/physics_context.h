#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/capsule_shape.h"
#include "physics/collision/shapes/concave_mesh_shape.h"
#include "physics/collision/shapes/convex_mesh_shape.h"
#include "physics/collision/shapes/height_field_shape.h"
#include "physics/collision/shapes/sphere_shape.h"
#include "physics/physics_world_settings.h"
#include "physics/types/convex_mesh.h"
#include "physics/types/half_edge_mesh.h"
#include "physics/types/height_field.h"
#include "physics/types/triangle_mesh.h"

namespace Vulkyrie {

    class BoxShape;
    class PhysicsWorld;

    class PhysicsContext final {
    public:
        PhysicsContext();

        VE_DELETE_MOVE_AND_COPY(PhysicsContext);

        ~PhysicsContext();

        [[nodiscard]] VE_INLINE HalfEdgeMesh &GetBoxShapeHalfEdgeMesh() {
            return _boxShapeHalfEdgeMesh;
        }

        [[nodiscard]] VE_INLINE HalfEdgeMesh &GetTriangleShapeHalfEdgeMesh() {
            return _triangleShapeHalfEdgeMesh;
        }

        PhysicsWorld *CreatePhysicsWorld(const PhysicsWorldSettings &worldSettings);
        void DestroyPhysicsWorld(PhysicsWorld *world);

        SphereShape *CreateSphereShape(const f32 radius);
        void DestroySphereShape(SphereShape *sphereShape);

        BoxShape *CreateBoxShape(const glm::vec3 &extent);
        void DestroyBoxShape(BoxShape *boxShape);

        CapsuleShape *CreateCapsuleShape(f32 radius, f32 height);
        void DestroyCapsuleShape(CapsuleShape *capsuleShape);

        ConvexMeshShape *CreateConvexMeshShape(ConvexMesh *convexMesh, const glm::vec3 &scaling = glm::vec3(1, 1, 1));
        void DestroyConvexMeshShape(ConvexMeshShape *convexMeshShape);

        HeightField *CreateHeightField(size_t nbGridColumns,
                                       size_t nbGridRows,
                                       const void *heightFieldData,
                                       HeightField::HeightDataType dataType,
                                       std::vector<Message> &messages,
                                       f32 integerHeightScale = 1.0f);
        void DestroyHeightField(HeightField *heightField);

        HeightFieldShape *CreateHeightFieldShape(HeightField *heightField, const glm::vec3 &scaling = glm::vec3(1, 1, 1));
        void DestroyHeightFieldShape(HeightFieldShape *heightFieldShape);

        ConcaveMeshShape *CreateConcaveMeshShape(TriangleMesh *triangleMesh, const glm::vec3 &scaling = glm::vec3(1, 1, 1));
        void DestroyConcaveMeshShape(ConcaveMeshShape *concaveMeshShape);

        // ConvexMesh *CreateConvexMesh(const PolygonVertexArray &polygonVertexArray, std::vector<Message> &messages);
        // ConvexMesh *CreateConvexMesh(const VertexArray &vertexArray, std::vector<Message> &messages);
        // void DestroyConvexMesh(ConvexMesh *convexMesh);
        //
        // TriangleMesh *CreateTriangleMesh(const TriangleVertexArray &triangleVertexArray, std::vector<Message> &messages);
        // void DestroyTriangleMesh(TriangleMesh *triangleMesh);

    private:
        std::unordered_set<PhysicsWorld *> _physicsWorlds;
        std::unordered_set<SphereShape *> _sphereShapes;
        std::unordered_set<BoxShape *> _boxShapes;
        std::unordered_set<CapsuleShape *> _capsuleShapes;
        std::unordered_set<ConvexMeshShape *> _convexMeshShapes;
        std::unordered_set<ConcaveMeshShape *> _concaveMeshShapes;
        std::unordered_set<HeightFieldShape *> _heightFieldShapes;
        std::unordered_set<ConvexMesh *> _convexMeshes;
        std::unordered_set<TriangleMesh *> _triangleMeshes;
        std::unordered_set<HeightField *> _heightFields;

        HalfEdgeMesh _boxShapeHalfEdgeMesh;
        HalfEdgeMesh _triangleShapeHalfEdgeMesh;

        /** @brief Populates the shared box half-edge mesh with the 8 vertices and 6 faces of a unit box and builds the half-edge connectivity. Vertex and
         * face ordering match `BoxShape::GetVertexPosition` and `BoxShape::GetFaceNormal` respectively, so half-edge topology queries on any `BoxShape`
         * resolve against the correct analytic data. */
        void initBoxShapeHalfEdgeMesh();

        /** @brief Populates the shared triangle half-edge mesh with 3 vertices and the two faces of a double-sided triangle (front face 0, back face 1,
         * matching `TriangleShape::GetFaceNormal`) and builds the half-edge connectivity. */
        void initTriangleShapeHalfEdgeMesh();
    };

} // namespace Vulkyrie
