#include "physics/systems/collision_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    CollisionSystem::CollisionSystem(PhysicsWorld &physicsWorld)
        : _physicsWorld(physicsWorld)
        , _colliderComponentStore(_physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(_physicsWorld.GetRigidBodyComponentStore()) {

        _capsuleVsCapsuleAlgorithm = new CapsuleVsCapsuleAlgorithm();
        _capsuleVsConvexPolyhedronAlgorithm = new CapsuleVsConvexPolyhedronAlgorithm();
        _convexPolyhedronVsConvexPolyhedronAlgorithm = new ConvexPolyhedronVsConvexPolyhedronAlgorithm();
        _sphereVsCapsuleAlgorithm = new SphereVsCapsuleAlgorithm();
        _sphereVsConvexPolyhedronAlgorithm = new SphereVsConvexPolyhedronAlgorithm();
        _sphereVsSphereAlgorithm = new SphereVsSphereAlgorithm();
    }

    CollisionSystem::~CollisionSystem() {
        delete _capsuleVsCapsuleAlgorithm;
        delete _capsuleVsConvexPolyhedronAlgorithm;
        delete _convexPolyhedronVsConvexPolyhedronAlgorithm;
        delete _sphereVsCapsuleAlgorithm;
        delete _sphereVsConvexPolyhedronAlgorithm;
        delete _sphereVsSphereAlgorithm;
    }

    void CollisionSystem::populateCollisionMatrix() {
        for (i32 i = 0; i < SUPPORTED_COLLISION_SHAPE_TYPE_COUNT; i++) {
            for (i32 j = 0; j < SUPPORTED_COLLISION_SHAPE_TYPE_COUNT; j++) {
                _collisionMatrix[i][j] = selectAlgorithm(i, j);
            }
        }
    }

    NarrowPhaseAlgorithm CollisionSystem::selectAlgorithm(i32 shapeOne, i32 shapeTwo) {
        CollisionShapeType shapeOneType = static_cast<CollisionShapeType>(shapeOne);
        CollisionShapeType shapeTwoType = static_cast<CollisionShapeType>(shapeTwo);

        if (shapeOne > shapeTwo) {
            return NarrowPhaseAlgorithm::NoCollisionCheck;
        }

        if (shapeOneType == CollisionShapeType::Sphere && shapeTwoType == CollisionShapeType::Sphere) {
            return NarrowPhaseAlgorithm::SphereVsSphere;
        }

        if (shapeOneType == CollisionShapeType::Sphere && shapeTwoType == CollisionShapeType::Capsule) {
            return NarrowPhaseAlgorithm::SphereVsCapsule;
        }

        if (shapeOneType == CollisionShapeType::Capsule && shapeTwoType == CollisionShapeType::Capsule) {
            return NarrowPhaseAlgorithm::CapsuleVsCapsule;
        }

        if (shapeOneType == CollisionShapeType::Sphere && shapeTwoType == CollisionShapeType::ConvexPolyhedron) {
            return NarrowPhaseAlgorithm::SphereVsConvexPolyhedron;
        }

        if (shapeOneType == CollisionShapeType::Capsule && shapeTwoType == CollisionShapeType::ConvexPolyhedron) {
            return NarrowPhaseAlgorithm::CapsuleVsConvexPolyhedron;
        }

        if (shapeOneType == CollisionShapeType::ConvexPolyhedron && shapeTwoType == CollisionShapeType::ConvexPolyhedron) {
            return NarrowPhaseAlgorithm::ConvexPolyhedronVsConvexPolyhedron;
        }

        return NarrowPhaseAlgorithm::NoCollisionCheck;
    }

    NarrowPhaseAlgorithm CollisionSystem::SelectNarrowPhaseAlgorithm(const CollisionShapeType shapeOne, const CollisionShapeType shapeTwo) const {
        u32 shapeOneIndex = static_cast<u32>(shapeOne);
        u32 shapeTwoIndex = static_cast<u32>(shapeTwo);

        VASSERT(shapeOneIndex < SUPPORTED_COLLISION_SHAPE_TYPE_COUNT && shapeTwoIndex < SUPPORTED_COLLISION_SHAPE_TYPE_COUNT, "Shape type(s) out of bounds.");

        if (shapeOneIndex > shapeTwoIndex) {
            return _collisionMatrix[shapeTwoIndex][shapeOneIndex];
        }

        return _collisionMatrix[shapeOneIndex][shapeTwoIndex];
    }

} // namespace Vulkyrie
