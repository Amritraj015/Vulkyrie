#include "physics/physics_context.h"

namespace Vulkyrie {

    PhysicsContext::PhysicsContext()
        : _boxShapeHalfEdgeMesh(6, 8, 24)
        , _triangleShapeHalfEdgeMesh(2, 3, 6) {
        initBoxShapeHalfEdgeMesh();
        initTriangleShapeHalfEdgeMesh();
    }

    void PhysicsContext::initBoxShapeHalfEdgeMesh() {
        // Vertex indices match the ordering of BoxShape::GetVertexPosition.
        for (size_t v = 0; v < 8; ++v) {
            [[maybe_unused]] const size_t vertexIndex = _boxShapeHalfEdgeMesh.AddVertex(v);
            VASSERT(vertexIndex == v, "Box shape half-edge mesh vertices must be added in order.");
        }

        // Faces are counter-clockwise as seen from outside the box, in the order of BoxShape::GetFaceNormal: +Z, +X, -Z, -X, -Y, +Y.
        _boxShapeHalfEdgeMesh.AddFace({ 0, 1, 2, 3 });
        _boxShapeHalfEdgeMesh.AddFace({ 1, 5, 6, 2 });
        _boxShapeHalfEdgeMesh.AddFace({ 5, 4, 7, 6 });
        _boxShapeHalfEdgeMesh.AddFace({ 4, 0, 3, 7 });
        _boxShapeHalfEdgeMesh.AddFace({ 4, 5, 1, 0 });
        _boxShapeHalfEdgeMesh.AddFace({ 3, 2, 6, 7 });

        _boxShapeHalfEdgeMesh.ComputeHalfEdges();

        VASSERT(_boxShapeHalfEdgeMesh.GetHalfEdgeCount() == 24, "Box shape half-edge mesh should have 24 half-edges after construction.");
    }

    void PhysicsContext::initTriangleShapeHalfEdgeMesh() {
        // Vertex indices match the ordering of TriangleShape::GetVertexPosition.
        for (size_t v = 0; v < 3; ++v) {
            [[maybe_unused]] const size_t vertexIndex = _triangleShapeHalfEdgeMesh.AddVertex(v);
            VASSERT(vertexIndex == v, "Triangle shape half-edge mesh vertices must be added in order.");
        }

        // Face 0 is the front face, face 1 the back face (reversed winding), matching TriangleShape::GetFaceNormal.
        _triangleShapeHalfEdgeMesh.AddFace({ 0, 1, 2 });
        _triangleShapeHalfEdgeMesh.AddFace({ 0, 2, 1 });

        _triangleShapeHalfEdgeMesh.ComputeHalfEdges();

        VASSERT(_triangleShapeHalfEdgeMesh.GetHalfEdgeCount() == 6, "Triangle shape half-edge mesh should have 6 half-edges after construction.");
    }

} // namespace Vulkyrie
