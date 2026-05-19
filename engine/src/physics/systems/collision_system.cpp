#include "physics/systems/collision_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    CollisionSystem::CollisionSystem(PhysicsWorld &physicsWorld)
        : _physicsWorld(physicsWorld)
        , _colliderComponentStore(_physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(_physicsWorld.GetRigidBodyComponentStore())
        , _collisionDispatch()
        , _overlappingPairs(physicsWorld, _nonCollidablePairs, _collisionDispatch)
        , _broadPhaseSystem(physicsWorld)
        , _narrowPhaseInput() {
    }

    void CollisionSystem::RemoveCollider(Collider &collider) {
        const i32 broadPhaseID = collider.GetBroadPhaseID();

        VASSERT(broadPhaseID != -1, "Collider does not have a valid broad-phase ID when trying to remove it from the collision system.");
        VASSERT(_broadPhaseIDToColliderEntityMap.contains(broadPhaseID), "Broad-phase ID does not exist in the map when trying to remove a collider.");

        const std::vector<u64> &overlappingPairs = _colliderComponentStore.GetOverlappingPairs(collider.GetEntity());

        while (overlappingPairs.size() > 0) {
            removeOverlappingPair(overlappingPairs[0], false);
        }

        _broadPhaseIDToColliderEntityMap.erase(broadPhaseID);
        _broadPhaseSystem.RemoveCollider(collider);
    }

    void CollisionSystem::AddNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity) {
        (void)bodyOneEntity;
        (void)bodyTwoEntity;
    }

    void CollisionSystem::RemoveNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity) {
        (void)bodyOneEntity;
        (void)bodyTwoEntity;
    }

    void CollisionSystem::NotifyOverlappingPairsToTestOverlap(Collider &collider) {
        (void)collider;
        // const std::vector<i32> &overlappingPairs = _colliderComponentStore.GetCollisionPairs(collider.GetEntity());
        //
        // for (const auto overlappingPair : overlappingPairs) {
        //     // Notify that the overlapping pair needs to be testbed for overlap
        //     _overlappingPairs.SetNeedToTestOverlap(overlappingPair, true);
        // }
    }

    void CollisionSystem::removeOverlappingPair(u64 pairID, bool notifyLostContact) {
        (void)pairID;
        (void)notifyLostContact;
    }

} // namespace Vulkyrie
