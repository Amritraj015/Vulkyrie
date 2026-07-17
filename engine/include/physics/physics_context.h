#pragma once

#include "physics/collision/shapes/capsule_shape.h"
#include "physics/collision/shapes/concave_mesh_shape.h"
#include "physics/collision/shapes/convex_mesh_shape.h"
#include "physics/collision/shapes/sphere_shape.h"
#include "physics/types/convex_mesh.h"
#include "physics/types/half_edge_mesh.h"
#include "physics/types/triangle_mesh.h"

namespace Vulkyrie {

    class BoxShape;
    class PhysicsWorld;

    class PhysicsContext {
    public:
        PhysicsContext();

        [[nodiscard]] VE_INLINE HalfEdgeMesh &GetBoxShapeHalfEdgeMesh() {
            return _boxShapeHalfEdgeMesh;
        }

        [[nodiscard]] VE_INLINE HalfEdgeMesh &GetTriangleShapeHalfEdgeMesh() {
            return _triangleShapeHalfEdgeMesh;
        }

    private:
        /** @brief Populates the shared box half-edge mesh with the 8 vertices and 6 faces of a unit box and builds the half-edge connectivity. Vertex and
         * face ordering match `BoxShape::GetVertexPosition` and `BoxShape::GetFaceNormal` respectively, so half-edge topology queries on any `BoxShape`
         * resolve against the correct analytic data. */
        void initBoxShapeHalfEdgeMesh();

        /** @brief Populates the shared triangle half-edge mesh with 3 vertices and the two faces of a double-sided triangle (front face 0, back face 1,
         * matching `TriangleShape::GetFaceNormal`) and builds the half-edge connectivity. */
        void initTriangleShapeHalfEdgeMesh();

        std::unordered_set<PhysicsWorld *> _physicsWorlds;
        std::unordered_set<SphereShape *> _sphereShapes;
        std::unordered_set<BoxShape *> _boxShapes;
        std::unordered_set<CapsuleShape *> _capsuleShapes;
        // std::unordered_set<ConvexMeshShape *> _convexMeshShapes;
        std::unordered_set<ConcaveMeshShape *> _concaveMeshShapes;
        // std::unordered_set<HeightFieldShape *> _heightFieldShapes;
        std::unordered_set<TriangleMesh *> _triangleMeshes;
        std::unordered_set<ConvexMesh *> _convexMeshes;

        HalfEdgeMesh _boxShapeHalfEdgeMesh;
        HalfEdgeMesh _triangleShapeHalfEdgeMesh;
    };

} // namespace Vulkyrie
