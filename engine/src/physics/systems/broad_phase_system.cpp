#include "physics/systems/broad_phase_system.h"
#include "physics/physics_world.h"
#include "physics/physics_constants.h"

namespace Vulkyrie {

    BroadPhaseSystem::BroadPhaseSystem(PhysicsWorld &physicsWorld)
        : _aabbTree()
        , _rigidBodyComponentStore(physicsWorld.GetRigidBodyComponentStore())
        , _colliderComponentStore(physicsWorld.GetColliderComponentStore())
        , _transformComponentStore(physicsWorld.GetTransformComponentStore())
        , _collisionSystem(physicsWorld.GetCollisionSystem()) {
        _movedShapes.reserve(AABB_TREE_DEFAULT_INITIAL_NODE_CAPACITY);
    }

    void BroadPhaseSystem::AddCollider(Collider &collider, const AABB &aabb) {
        VASSERT(collider.GetBroadPhaseID() == -1, "Collider is already in the broad phase system.");

        // Add the collider to the AABB tree.
        const i32 broadPhaseID = _aabbTree.AddObject(aabb, &collider);

        // Set the broad-phase ID for this collider in the collider component store.
        _colliderComponentStore.SetBroadPhaseID(collider.GetEntity(), broadPhaseID);

        // Add this collider to the set of moved colliders so that it will be included
        // in the next update of the broad-phase system and any overlapping pairs
        // involving this collider will be tested for overlap again.
        AddMovedCollider(broadPhaseID, collider);
    }

    void BroadPhaseSystem::RemoveCollider(Collider &collider) {
        VASSERT(collider.GetBroadPhaseID() != -1, "Collider is not in the broad phase system.");

        // Get the broad-phase ID for this collider from the collider component store.
        const i32 broadPhaseID = collider.GetBroadPhaseID();

        // Set the broad-phase ID for this collider to -1 in the collider component
        // store to indicate that it is no longer in the broad phase system.
        _colliderComponentStore.SetBroadPhaseID(collider.GetEntity(), -1);

        // Remove the collider object from the AABB tree using its broad-phase ID.
        _aabbTree.RemoveObject(broadPhaseID);

        // Remove this collider from the set of moved colliders.
        // WARN: This is an O(n) operation, but it should be rare for colliders to be removed
        // from the broad phase system, so it should not cause significant performance issues.
        _movedShapes.erase(std::remove(_movedShapes.begin(), _movedShapes.end(), broadPhaseID), _movedShapes.end());
    }

    void BroadPhaseSystem::UpdateCollider(Entity entity) {
        VASSERT(_colliderComponentStore.HasComponent(entity), "Entity does not have a ColliderComponent.");

        // Get the index of the collider component associated with this entity from the collider component store.
        const size_t index = _colliderComponentStore.GetEntityIndex(entity);

        // Update the state of this collider in the AABB tree based on its current transform and collision shape.
        updateColliderComponentStore(index, 1);
    }

    void BroadPhaseSystem::UpdateColliders() {
        const size_t activeColliderCount = _colliderComponentStore.GetActiveComponentCount();

        // Update the state of all active colliders in the collider component store.
        if (activeColliderCount > 0) {
            updateColliderComponentStore(0, activeColliderCount);
        }
    }

    void BroadPhaseSystem::AddMovedCollider(i32 broadPhaseID, Collider &collider) {
        VASSERT(broadPhaseID != -1, "Collider must already be in the broad phase system to be moved.");

        // Add the broad-phase ID of this moved collider to the set of moved colliders.
        _movedShapes.emplace_back(broadPhaseID);

        // Notify the collision system that this collider has moved and that any
        // overlapping pairs involving this collider should be tested for overlap again.
        _collisionSystem.NotifyOverlappingPairsToTestOverlap(collider);
    }

    bool BroadPhaseSystem::TestOverlap(i32 shapeOneBroadPhaseID, i32 shapeTwoBroadPhaseID) const {
        VASSERT(shapeOneBroadPhaseID != -1 && shapeTwoBroadPhaseID != -1, "Both colliders must be in the broad phase system to test for overlap.");

        // Get the Fat AABBs of the two shapes from the AABB tree.
        const AABB &aabbOne = _aabbTree.GetFatAABB(shapeOneBroadPhaseID);
        const AABB &aabbTwo = _aabbTree.GetFatAABB(shapeTwoBroadPhaseID);

        // Check if these shapes overlap.
        return aabbOne.CollidesWith(aabbTwo);
    }

    void BroadPhaseSystem::ComputeOverlappingPairs(std::vector<std::pair<i32, i32>> &outOverlappingPairs) {
        // Query the AABB tree for overlapping pairs involving the moved shapes and populate the provided vector with the results.
        _aabbTree.QueryOverlappingPairs(_movedShapes, outOverlappingPairs);

        // After computing the overlapping pairs, we can clear the set of moved shapes since we have now
        // accounted for their movement in the broad-phase system and any new overlaps have been identified.
        _movedShapes.clear();
    }

    void BroadPhaseSystem::updateColliderComponentStore(size_t startIndex, size_t count) {
        VASSERT(count > 0, "Count must be greater than 0.");
        VASSERT(startIndex < _colliderComponentStore.GetTotalComponentCount(), "Start index must be within the bounds of the collider component store.");
        VASSERT(startIndex + count <= _colliderComponentStore.GetTotalComponentCount(),
                "Start index and count must be within the bounds of the collider component store.");

        // Iterate over the specified range of collider components in the collider component store to update their corresponding entries in the AABB tree.
        for (size_t i = startIndex; i < startIndex + count; i++) {
            // Get the broad-phase ID for this collider from the collider component store.
            const i32 broadPhaseID = _colliderComponentStore.GetBroadPhaseIDAtIndex(i);

            // If the broad-phase ID is -1, this collider has not yet been registered with the broad-phase system, so we skip it.
            if (-1 != broadPhaseID) {

                // Get the body entity associated with this collider from the collider component store.
                const Entity bodyEntity = _colliderComponentStore.GetBodyEntityAtIndex(i);

                // Get the transform component of the body entity from the transform component store.
                const TransformComponent &transform = _transformComponentStore.GetTransform(bodyEntity);

                // Get the local-to-body transform for this collider from the collider component store.
                const TransformComponent &localToBodyTransform = _colliderComponentStore.GetLocalToBodyTransformAtIndex(i);

                // Get the collision shape for this collider from the collider component store and
                // compute its transformed AABB using the body's transform and the local-to-body transform.
                const AABB aabb = _colliderComponentStore.GetCollisionShapeAtIndex(i).ComputeTransformedAABB(transform * localToBodyTransform);

                // Check if the collider's collision shape has changed size since the last update.
                const bool forceReInsert = _colliderComponentStore.HasCollisionShapeChangedSizeAtIndex(i);

                // If it has, we need to force a re-insert into the AABB tree to ensure that the tree
                // structure is updated correctly to accommodate the new size of the collider's shape.
                const bool wasReInserted = _aabbTree.UpdateObject(broadPhaseID, aabb, forceReInsert);

                // If the collider was re-inserted, we need to add it to the set of moved colliders
                // so that the collision system will be notified to check for new overlaps.
                if (wasReInserted) {
                    AddMovedCollider(broadPhaseID, _colliderComponentStore.GetColliderAtIndex(i));
                }

                // After updating the collider in the AABB tree, we can reset the collision shape changed
                // size flag for this collider in the collider component store, since we have now accounted
                // for the size change in the broad-phase system.
                _colliderComponentStore.SetCollisionShapeChangedSizeAtIndex(i, false);
            }
        }
    }

} // namespace Vulkyrie
