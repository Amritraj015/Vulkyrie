#include "physics/physics_context.h"
#include "physics/collision/shapes/box_shape.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    PhysicsContext::PhysicsContext()
        : _boxShapeHalfEdgeMesh(6, 8, 24)
        , _triangleShapeHalfEdgeMesh(2, 3, 6) {
        initBoxShapeHalfEdgeMesh();
        initTriangleShapeHalfEdgeMesh();
    }

    PhysicsContext::~PhysicsContext() {
        // Destroy the physics worlds.
        while (!_physicsWorlds.empty()) {
            DestroyPhysicsWorld(*_physicsWorlds.begin());
        }

        // Destroy the sphere shapes.
        while (!_sphereShapes.empty()) {
            DestroySphereShape(*_sphereShapes.begin());
        }

        // Destroy the box shapes.
        while (!_boxShapes.empty()) {
            DestroyBoxShape(*_boxShapes.begin());
        }

        // Destroy the capsule shapes.
        while (!_capsuleShapes.empty()) {
            DestroyCapsuleShape(*_capsuleShapes.begin());
        }

        // Destroy the convex mesh shapes.
        while (!_convexMeshShapes.empty()) {
            DestroyConvexMeshShape(*_convexMeshShapes.begin());
        }

        // Destroy the heigh-field shapes.
        while (!_heightFieldShapes.empty()) {
            DestroyHeightFieldShape(*_heightFieldShapes.begin());
        }

        // Destroy the concave mesh shapes.
        while (!_concaveMeshShapes.empty()) {
            DestroyConcaveMeshShape(*_concaveMeshShapes.begin());
        }

        // Destroy the convex meshes.
        while (!_convexMeshes.empty()) {
            auto *convexMesh = *_convexMeshes.begin();
            _convexMeshes.erase(convexMesh);

            delete convexMesh;
        }

        // Destroy the triangle meshes.
        while (!_triangleMeshes.empty()) {
            auto *triangleMesh = *_triangleMeshes.begin();
            _triangleMeshes.erase(triangleMesh);

            delete triangleMesh;
        }

        // Destroy the height-fields.
        while (!_heightFields.empty()) {
            DestroyHeightField(*_heightFields.begin());
        }
    }

    PhysicsWorld *PhysicsContext::CreatePhysicsWorld(const PhysicsWorldSettings &worldSettings) {
        auto *world = new PhysicsWorld(worldSettings);

        _physicsWorlds.insert(world);

        return world;
    }

    void PhysicsContext::DestroyPhysicsWorld(PhysicsWorld *world) {
        _physicsWorlds.erase(world);

        delete world;
    }

    SphereShape *PhysicsContext::CreateSphereShape(const f32 radius) {
        if (radius <= f32(0.0)) {
            VERROR("Circle Radius cannot be <= 0, provided radius: {}", radius);

            return nullptr;
        }

        auto *shape = new SphereShape(radius);
        _sphereShapes.insert(shape);

        return shape;
    }

    void PhysicsContext::DestroySphereShape(SphereShape *sphereShape) {
        _sphereShapes.erase(sphereShape);

        delete sphereShape;
    }

    BoxShape *PhysicsContext::CreateBoxShape(const glm::vec3 &extent) {
        if (extent.x <= f32(0.0) || extent.y <= f32(0.0) || extent.z <= f32(0.0)) {
            VERROR("Invalid half extents x: {}, y: {}, z: {}", extent.x, extent.y, extent.z);

            return nullptr;
        }

        auto *shape = new BoxShape(extent, *this);

        _boxShapes.insert(shape);

        return shape;
    }

    void PhysicsContext::DestroyBoxShape(BoxShape *boxShape) {
        _boxShapes.erase(boxShape);

        delete boxShape;
    }

    CapsuleShape *PhysicsContext::CreateCapsuleShape(f32 radius, f32 height) {
        if (radius <= f32(0.0)) {
            VERROR("Capsule radius must be > 0, provided radius: {}", radius);

            return nullptr;
        }

        if (height <= f32(0.0)) {
            VERROR("Capsule height must be > 0, provided height: {}", height);

            return nullptr;
        }

        auto *shape = new CapsuleShape(radius, height);

        _capsuleShapes.insert(shape);

        return shape;
    }

    void PhysicsContext::DestroyCapsuleShape(CapsuleShape *capsuleShape) {
        _capsuleShapes.erase(capsuleShape);

        delete capsuleShape;
    }

    ConvexMeshShape *PhysicsContext::CreateConvexMeshShape(ConvexMesh *convexMesh, const glm::vec3 &scaling) {
        auto *shape = new ConvexMeshShape(convexMesh, scaling);

        _convexMeshShapes.insert(shape);

        return shape;
    }

    void PhysicsContext::DestroyConvexMeshShape(ConvexMeshShape *convexMeshShape) {
        _convexMeshShapes.erase(convexMeshShape);

        delete convexMeshShape;
    }

    HeightField *PhysicsContext::CreateHeightField(size_t nbGridColumns,
                                                   size_t nbGridRows,
                                                   const void *heightFieldData,
                                                   HeightField::HeightDataType dataType,
                                                   std::vector<Message> &messages,
                                                   f32 integerHeightScale) {
        auto *heightField = new HeightField(_triangleShapeHalfEdgeMesh);

        const bool isValid = heightField->Initialize(nbGridColumns, nbGridRows, heightFieldData, dataType, messages, integerHeightScale);

        if (!isValid) {
            delete heightField;

            return nullptr;
        }

        _heightFields.insert(heightField);

        return heightField;
    }

    void PhysicsContext::DestroyHeightField(HeightField *heightField) {
        _heightFields.erase(heightField);

        delete heightField;
    }

    HeightFieldShape *PhysicsContext::CreateHeightFieldShape(HeightField *heightField, const glm::vec3 &scaling) {
        auto *shape = new HeightFieldShape(heightField, scaling);

        _heightFieldShapes.insert(shape);

        return shape;
    }

    void PhysicsContext::DestroyHeightFieldShape(HeightFieldShape *heightFieldShape) {
        _heightFieldShapes.erase(heightFieldShape);

        delete heightFieldShape;
    }

    ConcaveMeshShape *PhysicsContext::CreateConcaveMeshShape(TriangleMesh *triangleMesh, const glm::vec3 &scaling) {
        auto *shape = new ConcaveMeshShape(triangleMesh, _triangleShapeHalfEdgeMesh, scaling);

        _concaveMeshShapes.insert(shape);

        return shape;
    }

    void PhysicsContext::DestroyConcaveMeshShape(ConcaveMeshShape *concaveMeshShape) {
        _concaveMeshShapes.erase(concaveMeshShape);

        delete concaveMeshShape;
    }

    // ConvexMesh *CreateConvexMesh(const PolygonVertexArray &polygonVertexArray, std::vector<Message> &messages);
    // ConvexMesh *CreateConvexMesh(const VertexArray &vertexArray, std::vector<Message> &messages);
    // void DestroyConvexMesh(ConvexMesh *convexMesh);
    //
    // TriangleMesh *CreateTriangleMesh(const TriangleVertexArray &triangleVertexArray, std::vector<Message> &messages);
    // void DestroyTriangleMesh(TriangleMesh *triangleMesh);

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
