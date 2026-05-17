#pragma once

#include "physics/collision/broadphase/dynamic_aabb_tree.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/transform_component_store.h"
#include "physics/components/rigid_body_component_store.h"

namespace Vulkyrie {

    class CollisionSystem;
    class PhysicsWorld;

    /** @brief The BroadPhaseSystem is responsible for managing the broad phase of collision detection in the physics simulation. It uses a dynamic AABB tree to
     * efficiently organize colliders in the physics world and quickly identify potential collisions between entities. The broad phase system interacts with the
     * collider, transform, and rigid body component stores to maintain accurate AABBs for colliders and to track which colliders have moved and need to be
     * updated in the broad phase system. It also works closely with the collision system to notify it of potential overlaps between colliders that need to be
     * tested for actual collisions in the narrow phase. The BroadPhaseSystem class provides methods for adding, removing, and updating colliders in the broad
     * phase system, as well as for testing overlaps and computing overlapping pairs of colliders for collision detection. */
    class BroadPhaseSystem final {
    public:
        /** @brief Constructs an instance of BroadPhaseSystem with references to the necessary component stores and collision system from the physics world.
         * The constructor initializes the dynamic AABB tree and sets up the references to the rigid body, collider, and transform component stores, as well
         * as the collision system, which are essential for managing colliders and their AABBs in the broad phase of collision detection. The
         * BroadPhaseSystem relies on these component stores to access and update collider information, and it interacts with the collision system to notify
         * it of potential overlaps that need to be tested for collisions.
         * @param physicsWorld The physics world that this broad phase system belongs to. */
        BroadPhaseSystem(PhysicsWorld &physicsWorld);

        // Delete the copy constructor and copy assignment operator.
        BroadPhaseSystem(const BroadPhaseSystem &) = delete;
        BroadPhaseSystem &operator=(const BroadPhaseSystem &) = delete;

        // Delete the move constructor and move assignment operator.
        BroadPhaseSystem(BroadPhaseSystem &&) = delete;
        BroadPhaseSystem &operator=(BroadPhaseSystem &&) = delete;

        /** @brief Destructor for BroadPhaseSystem. */
        ~BroadPhaseSystem() = default;

        /** @brief Adds a collider to the broad phase system by inserting its AABB into the dynamic AABB tree. The broad phase
         * system will use this AABB tree to efficiently identify potential collisions between colliders during collision detection. The method also
         * assigns a broad phase ID to the collider, which can be used for tracking and updating the collider's position in the broad phase system as it
         * moves or changes shape during the simulation.
         * @param collider The collider to be added to the broad phase system. This collider must have a valid AABB that represents its collision geometry
         * in world space.
         * @param aabb The axis-aligned bounding box (AABB) that represents the collision geometry of the collider in world space. This AABB is used for
         * efficient broad phase collision detection and should encompass the entire shape of the collider. */
        void AddCollider(Collider &collider, const AABB &aabb);

        /** @brief Removes a collider from the broad phase system by removing its AABB from the dynamic AABB tree. This method should be called when a
         * collider is removed from the physics world or when it is deactivated, to ensure that it is no longer considered for collision detection in the
         * broad phase. The method uses the collider's broad phase ID to identify and remove its corresponding AABB from the tree, effectively excluding it
         * from future broad phase collision checks.
         * @param collider The collider to be removed from the broad phase system. This collider must have a valid broad phase ID that corresponds to its
         * entry in the dynamic AABB tree. */
        void RemoveCollider(Collider &collider);

        /** @brief Updates the position of a collider in the broad phase system by updating its AABB in the dynamic AABB tree. This method should be called
         * whenever a collider has moved or changed shape in a way that affects its AABB, to ensure that the broad phase system has the latest information
         * about the collider's position and can accurately identify potential collisions. The method retrieves the current AABB of the collider and updates
         * its position in the tree using the collider's broad phase ID, which allows the broad phase system to efficiently track and manage colliders as
         * they move and interact in the physics simulation.
         * @param entity The entity associated with the collider that needs to be updated in the broad phase system. The entity must have a
         * ColliderComponent associated with it, and the corresponding collider must have a valid broad phase ID for this update to work correctly. */
        void UpdateCollider(Entity entity);

        /** @brief Updates the state of all broad-phase colliders, ensuring that any colliders that have
         * moved or changed shape are properly updated in the dynamic AABB tree and the broad-phase system. */
        void UpdateColliders();

        /** @brief Adds a collider to the set of moved colliders that need to be updated in the broad phase system. This method should be called whenever a
         * collider has moved or changed shape in a way that affects its AABB, to ensure that the broad phase system is aware of the change and can update
         * the collider's position in the dynamic AABB tree accordingly. The method uses the collider's broad phase ID to track which colliders have been
         * moved and need to be updated in the next broad phase update cycle.
         * @param broadPhaseID The broad phase ID of the collider that has been moved and needs to be updated in the broad phase system. This ID corresponds
         * to the entry of the collider in the dynamic AABB tree, and it must be valid and currently active in the broad phase system. */
        void AddMovedCollider(i32 broadPhaseID, Collider &collider);

        /** @brief Tests whether the fat AABBs of two colliders, identified by their broad phase IDs, overlap in the broad phase system. This method
         * retrieves the fat AABBs associated with the specified broad phase IDs from the dynamic AABB tree and checks for overlap between them. If the
         * AABBs overlap, it indicates that the corresponding colliders may be colliding and should be further tested for actual collisions in the narrow
         * phase of the collision detection system. The method returns true if there is an overlap between the AABBs of the two colliders, and false
         * otherwise.
         * @param shapeOneBroadPhaseID The broad phase ID of the first collider to be tested for overlap. This ID corresponds to the entry of the collider
         * in the dynamic AABB tree, and it must be valid and currently active in the broad phase system.
         * @param shapeTwoBroadPhaseID The broad phase ID of the second collider to be tested for overlap. This ID corresponds to the entry of the collider
         * in the dynamic AABB tree, and it must be valid and currently active in the broad phase system.
         * @return True if there is an overlap between the AABBs of the two colliders identified by their broad phase IDs, indicating a potential collision,
         * and false otherwise. */
        bool TestOverlap(i32 shapeOneBroadPhaseID, i32 shapeTwoBroadPhaseID) const;

        /** @brief Computes the pairs of colliders that are potentially overlapping in the broad phase system. This method queries the dynamic AABB tree to
         * identify pairs of colliders whose AABBs overlap, indicating that they may be colliding and need to be tested for actual collisions in the narrow
         * phase. The method populates the provided vector with pairs of broad phase IDs corresponding to the potentially overlapping colliders. Each pair
         * in the vector represents two colliders that have overlapping AABBs and should be further tested for collision detection and response in the
         * narrow phase of the collision system.
         * @param outOverlappingPairs A reference to a vector that will be populated with pairs of broad phase IDs corresponding to potentially overlapping
         * colliders. Each pair in the vector represents two colliders that have overlapping AABBs and should be further tested for collision detection and
         * response in the narrow phase. */
        void ComputeOverlappingPairs(std::vector<std::pair<i32, i32>> &outOverlappingPairs);

        /** @brief Retrieves the fat AABB associated with a specified broad phase ID from the dynamic AABB tree. The fat AABB is an expanded version of the
         * collider's original AABB that accounts for potential movement and changes in shape during the simulation. It is used in the broad phase collision
         * detection to ensure that potential collisions are not missed due to small movements or changes in the collider's geometry. The method returns a
         * reference to the fat AABB, which can be used for efficient collision checks against other colliders in the broad phase system.
         * @param broadPaseID The broad phase ID of the collider whose fat AABB is to be retrieved. This ID corresponds to the entry of the collider in the
         * dynamic AABB tree, and it must be valid and currently active in the broad phase system.
         * @return A reference to the fat AABB associated with the specified broad phase ID. This AABB can be used for efficient collision checks against
         * other colliders in the broad phase system. */
        [[nodiscard]] VE_FORCE_INLINE const AABB &GetFatAABB(i32 broadPaseID) const {
            return _aabbTree.GetFatAABB(broadPaseID);
        }

    private:
        /** @brief Dynamic AABB tree used for broad phase collision detection. This data structure efficiently organizes the axis-aligned bounding boxes
         * (AABBs) of colliders in the physics world, allowing for fast queries to identify potential collisions between entities. The dynamic AABB tree
         * supports insertion, removal, and updating of AABBs as colliders move or change shape during the simulation. It is a key component of the broad
         * phase system, which quickly culls pairs of colliders that are not likely to collide, reducing the number of narrow phase collision checks needed
         * for accurate collision detection and response. */
        DynamicAABBTree _aabbTree;

        /** @brief A reference to the RigidBodyComponentStore. */
        RigidBodyComponentStore &_rigidBodyComponentStore;

        /** @brief A reference to the ColliderComponentStore. */
        ColliderComponentStore &_colliderComponentStore;

        /** @brief A reference to the TransformComponentStore. */
        TransformComponentStore &_transformComponentStore;

        /** @brief A reference to the CollisionSystem */
        CollisionSystem &_collisionSystem;

        /** @brief A buffer that stores the broad phase IDs of colliders that have been moved and need to be updated in the broad phase system. This buffer
         * is used to track which colliders have changed position or shape during the simulation, allowing the broad phase system to efficiently update
         * their entries in the dynamic AABB tree and identify new potential collisions. The buffer is cleared after each update cycle, once all moved
         * colliders have been processed and their overlaps have been computed. */
        std::vector<i32> _movedShapes;

        /** @brief Updates the entries in the collider component store for a specified range of components. This method is called to update the AABBs of
         * colliders in the broad phase system when their transforms or shapes have changed. It iterates over the specified range of collider components,
         * retrieves their associated body entities, transforms, local-to-body transforms, and collision shapes, computes their transformed AABBs, and
         * updates their entries in the dynamic AABB tree accordingly. If a collider's shape has changed size, it forces a re-insert into the AABB tree to
         * ensure that the tree structure is updated correctly to accommodate the new size of the collider's shape. This method is essential for maintaining
         * accurate AABBs for colliders in the broad phase system and ensuring that potential collisions are correctly identified during collision
         * detection.
         * @param startIndex The starting index of the range of collider components to be updated. Must be within the bounds of the collider component
         * store.
         * @param count The number of collider components to be updated starting from startIndex. Must be greater than 0 and within the bounds of the
         * collider component store. */
        void updateColliderComponentStore(size_t startIndex, size_t count);
    };

} // namespace Vulkyrie
