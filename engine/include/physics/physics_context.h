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

    class PhysicsContext {
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

        PhysicsWorld *createPhysicsWorld(const PhysicsWorldSettings &worldSettings);
        void destroyPhysicsWorld(PhysicsWorld *world);

        SphereShape *createSphereShape(const f32 radius);
        void destroySphereShape(SphereShape *sphereShape);

        BoxShape *createBoxShape(const glm::vec3 &extent);
        void destroyBoxShape(BoxShape *boxShape);

        CapsuleShape *createCapsuleShape(f32 radius, f32 height);
        void destroyCapsuleShape(CapsuleShape *capsuleShape);

        ConvexMeshShape *createConvexMeshShape(ConvexMesh *convexMesh, const glm::vec3 &scaling = glm::vec3(1, 1, 1));
        void destroyConvexMeshShape(ConvexMeshShape *convexMeshShape);

        HeightField *createHeightField(size_t nbGridColumns,
                                       size_t nbGridRows,
                                       const void *heightFieldData,
                                       HeightField::HeightDataType dataType,
                                       std::vector<Message> &messages,
                                       f32 integerHeightScale = 1.0f);
        void destroyHeightField(HeightField *heightField);

        HeightFieldShape *createHeightFieldShape(HeightField *heightField, const glm::vec3 &scaling = glm::vec3(1, 1, 1));
        void destroyHeightFieldShape(HeightFieldShape *heightFieldShape);

        ConcaveMeshShape *createConcaveMeshShape(TriangleMesh *triangleMesh, const glm::vec3 &scaling = glm::vec3(1, 1, 1));
        void destroyConcaveMeshShape(ConcaveMeshShape *concaveMeshShape);

        // ConvexMesh *createConvexMesh(const PolygonVertexArray &polygonVertexArray, std::vector<Message> &messages);
        // ConvexMesh *createConvexMesh(const VertexArray &vertexArray, std::vector<Message> &messages);
        // void destroyConvexMesh(ConvexMesh *convexMesh);
        //
        // TriangleMesh *createTriangleMesh(const TriangleVertexArray &triangleVertexArray, std::vector<Message> &messages);
        // void destroyTriangleMesh(TriangleMesh *triangleMesh);

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

        void deletePhysicsWorld(PhysicsWorld *world);
        void deleteSphereShape(SphereShape *sphereShape);
        void deleteBoxShape(BoxShape *boxShape);
        void deleteCapsuleShape(CapsuleShape *capsuleShape);
        void deleteConvexMeshShape(ConvexMeshShape *convexMeshShape);
        void deleteHeightFieldShape(HeightFieldShape *heightFieldShape);
        void deleteConcaveMeshShape(ConcaveMeshShape *concaveMeshShape);
        void deleteConvexMesh(ConvexMesh *convexMesh);
        void deleteTriangleMesh(TriangleMesh *triangleMesh);
        void deleteHeightField(HeightField *heightField);
    };

} // namespace Vulkyrie
