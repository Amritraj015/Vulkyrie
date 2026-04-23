#pragma once

#include "core/entity.h"
#include "physics/collision/collider.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    /** @brief Plain data bag passed to ColliderComponentStore::AddComponent. All fields are copied into the store's parallel arrays on construction; the
     * struct is not stored itself and may be constructed on the stack. */
    struct ColliderComponent final {
        public:
            /** @brief The entity of the rigid body to which this collider is attached. */
            Entity BodyEntity;

            /** @brief Pointer to the Collider object that owns this component. Must remain valid for the lifetime of the component. */
            Vulkyrie::Collider *Collider;

            /** @brief Transform from the collider's local space to the owning body's local space. */
            TransformComponent LocalToBodyTransform;

            /** @brief Pointer to the geometric shape used for collision detection. Must remain valid for the lifetime of the component. */
            Vulkyrie::CollisionShape *CollisionShape;

            /** @brief Bitmask identifying the collision category this collider belongs to. Used to filter collision pairs against CollidesWithMaskBits. */
            u16 CollisionCategoryBits;

            /** @brief Bitmask of the collision categories this collider should respond to. A collision is considered only when
             * (this->CollisionCategoryBits & other->CollidesWithMaskBits) != 0 and vice versa. */
            u16 CollidesWithMaskBits;

            /** @brief Transform from the collider's local space to world space, updated each simulation step. */
            TransformComponent LocalToWorldTransform;

            /** @brief Physical material properties (friction, restitution, etc.) for this collider. */
            Vulkyrie::Material Material;

            /** @brief Constructs a ColliderComponent with the given parameters. All reference parameters are copied by value.
             * @param bodyEntity The entity of the rigid body to which this collider is attached.
             * @param collider Pointer to the owning Collider object.
             * @param localToBodyTransform Transform from the collider's local space to the body's local space.
             * @param collisionShape Pointer to the geometric collision shape.
             * @param collisionCategoryBits Bitmask for the category this collider belongs to.
             * @param collidesWithMaskBits Bitmask of categories this collider responds to.
             * @param localToWorldTransform Transform from the collider's local space to world space.
             * @param material Physical material properties for this collider. */
            ColliderComponent(Entity bodyEntity,
                              Vulkyrie::Collider *collider,
                              const TransformComponent &localToBodyTransform,
                              Vulkyrie::CollisionShape *collisionShape,
                              u16 collisionCategoryBits,
                              u16 collidesWithMaskBits,
                              const TransformComponent &localToWorldTransform,
                              const Vulkyrie::Material &material)
                : BodyEntity(bodyEntity)
                , Collider(collider)
                , LocalToBodyTransform(localToBodyTransform)
                , CollisionShape(collisionShape)
                , CollisionCategoryBits(collisionCategoryBits)
                , CollidesWithMaskBits(collidesWithMaskBits)
                , LocalToWorldTransform(localToWorldTransform)
                , Material(material) {
            }
    };

    /** @brief The ColliderComponentStore is responsible for managing ColliderComponents associated with entities. It maintains a dense packing of active
     * components at the front of the storage vector for efficient iteration, while allowing for dynamic addition, removal, activation, and deactivation of
     * components without fragmentation. The manager uses a mapping from entities to component indices to enable fast lookups and updates. Each
     * ColliderComponent contains all the necessary data for collision detection and response in the physics simulation, including references to the associated
     * Collider, CollisionShape, and Material, as well as transformation data for converting between local and world space. */
    class ColliderComponentStore final : public ComponentStore {
        public:
            /** @brief Constructs an instance of ColliderComponentStore. */
            ColliderComponentStore();

            // Delete the copy constructor and copy assignment operator.
            ColliderComponentStore(const ColliderComponentStore &) = delete;
            ColliderComponentStore &operator=(const ColliderComponentStore &) = delete;

            // Delete the move constructor and move assignment operator.
            ColliderComponentStore(ColliderComponentStore &&) = delete;
            ColliderComponentStore &operator=(ColliderComponentStore &&) = delete;

            /** @brief Destructor for ColliderComponentStore. */
            ~ColliderComponentStore() override = default;

            /** @brief Adds a ColliderComponent to the specified entity. Active components are stored at the front of the vector and inactive ones at the back
             * to maintain dense packing for efficient iteration.
             * @param entity The entity to which the ColliderComponent will be added. Must not already have a ColliderComponent.
             * @param component The ColliderComponent to be added to the entity.
             * @param active Whether the entity is currently active. */
            void AddComponent(Entity entity, const ColliderComponent &component, bool active);

            /** @brief Retrieves the body entity associated with the specified collider entity. The body entity represents the parent body to which the collider
             * is attached in the physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose body entity is to be retrieved.
             * @return The body entity associated with the specified collider entity. This entity can be used to access the parent body of the collider in the
             * physics simulation. */
            [[nodiscard]] VE_FORCE_INLINE Entity GetBodyEntity(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _bodyEntities[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Retrieves a reference to the Collider associated with the specified collider entity. The Collider represents the collision properties and
             * behavior of the entity in the physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose Collider is to be retrieved.
             * @return A reference to the Collider associated with the specified collider entity. This Collider can be used to access and modify the collision
             * properties and behavior of the entity in the physics simulation. */
            [[nodiscard]]
            VE_FORCE_INLINE Collider &GetCollider(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return *_colliders[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Retrieves a reference to the local-to-body transform associated with the specified collider entity. The local-to-body transform
             * represents the transformation that converts coordinates from the collider's local space to the body's local space, which is essential for
             * accurate collision detection and response in the physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose local-to-body transform is to be retrieved.
             * @return A reference to the local-to-body transform associated with the specified collider entity. This transform can be used to convert
             * coordinates from the collider's local space to the body's local space for accurate collision detection and response. */
            [[nodiscard]] VE_FORCE_INLINE const TransformComponent &GetLocalToBodyTransform(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _localToBodyTransforms[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Sets the local-to-body transform for the specified collider entity. The local-to-body transform represents the transformation that
             * converts coordinates from the collider's local space to the body's local space, which is essential for accurate collision detection and response
             * in the physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose local-to-body transform is to be set.
             * @param transform The new local-to-body transform to be set for the specified collider entity. */
            VE_FORCE_INLINE void SetLocalToBodyTransform(Entity colliderEntity, const TransformComponent &transform) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _localToBodyTransforms[_entityToComponentIndex.find(colliderEntity)->second] = transform;
            }

            /** @brief Retrieves a reference to the collision shape associated with the specified collider entity. The collision shape defines the geometric
             * representation of the collider used for collision detection and response in the physics simulation. The entity must have a ColliderComponent
             * associated with it.
             * @param colliderEntity The entity of the collider whose collision shape is to be retrieved.
             * @return A reference to the collision shape associated with the specified collider entity. This shape can be used for collision detection and
             * response calculations in the physics simulation. */
            [[nodiscard]] VE_FORCE_INLINE CollisionShape &GetCollisionShape(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return *_collisionShapes[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Retrieves the broad-phase ID assigned to the specified collider entity. The broad-phase ID is used by the broad-phase collision
             * detection system to track the collider's bounding volume. A value of -1 indicates the collider has not yet been registered with the broad phase.
             * The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose broad-phase ID is to be retrieved.
             * @return The broad-phase ID of the collider, or -1 if not yet registered. */
            [[nodiscard]] VE_FORCE_INLINE i32 GetBroadPhaseID(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _broadPhaseIDs[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Sets the broad-phase ID for the specified collider entity. This is called by the broad-phase system when the collider is inserted into
             * the broad-phase structure. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose broad-phase ID is to be set.
             * @param broadPhaseID The broad-phase ID assigned by the broad-phase collision detection system. */
            VE_FORCE_INLINE void SetBroadPhaseID(Entity colliderEntity, i32 broadPhaseID) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _broadPhaseIDs[_entityToComponentIndex.find(colliderEntity)->second] = broadPhaseID;
            }

            /** @brief Retrieves the collision category bitmask for the specified collider entity. The category bits define the collision group this collider
             * belongs to. Collision filtering is performed by testing this value against the CollidesWithMaskBits of the other collider. The entity must have
             * a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose collision category bits are to be retrieved.
             * @return The collision category bitmask assigned to this collider. */
            [[nodiscard]] VE_FORCE_INLINE u16 GetCollisionCategoryBits(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _collisionCategoryBits[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Sets the collision category bitmask for the specified collider entity. The category bits define the collision group this collider belongs
             * to and are used during broad-phase and narrow-phase filtering. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose collision category bits are to be set.
             * @param collisionCategoryBits The new collision category bitmask to assign to this collider. */
            VE_FORCE_INLINE void SetCollisionCategoryBits(Entity colliderEntity, u16 collisionCategoryBits) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _collisionCategoryBits[_entityToComponentIndex.find(colliderEntity)->second] = collisionCategoryBits;
            }

            /** @brief Retrieves the collides-with bitmask for the specified collider entity. This mask defines which collision categories this collider will
             * respond to. A collision is only processed when (this->CollisionCategoryBits & other->CollidesWithMaskBits) != 0 and vice versa. The entity must
             * have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose collides-with mask bits are to be retrieved.
             * @return The collides-with bitmask for the specified collider. */
            [[nodiscard]] VE_FORCE_INLINE u16 GetCollidesWithMaskBits(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _collidesWithMaskBits[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Sets the collides-with bitmask for the specified collider entity. This mask defines which collision categories this collider will respond
             * to. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose collides-with mask bits are to be set.
             * @param maskBits The new collides-with bitmask to assign to this collider. */
            VE_FORCE_INLINE void SetCollidesWithMaskBits(Entity colliderEntity, u16 maskBits) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _collidesWithMaskBits[_entityToComponentIndex.find(colliderEntity)->second] = maskBits;
            }

            /** @brief Retrieves a reference to the local-to-world transform associated with the specified collider entity. The local-to-world transform
             * represents the transformation that converts coordinates from the collider's local space to world space, which is essential for accurate collision
             * detection and response in the physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose local-to-world transform is to be retrieved.
             * @return A reference to the local-to-world transform associated with the specified collider entity. This transform can be used to convert
             * coordinates from the collider's local space to world space for accurate collision detection and response. */
            [[nodiscard]] VE_FORCE_INLINE const TransformComponent &GetLocalToWorldTransform(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _localToWorldTransforms[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Sets the local-to-world transform for the specified collider entity. The local-to-world transform represents the transformation that
             * converts coordinates from the collider's local space to world space, which is essential for accurate collision detection and response in the
             * physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose local-to-world transform is to be set.
             * @param transform The new local-to-world transform to be set for the specified collider entity. */
            VE_FORCE_INLINE void SetLocalToWorldTransform(Entity colliderEntity, const TransformComponent &transform) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _localToWorldTransforms[_entityToComponentIndex.find(colliderEntity)->second] = transform;
            }

            /** @brief Retrieves a reference to the vector of collision pairs associated with the specified collider entity. Collision pairs represent the pairs
             * of colliders that are currently colliding or have recently collided with the specified collider. The entity must have a ColliderComponent
             * associated with it.
             * @param colliderEntity The entity of the collider whose collision pairs are to be retrieved.
             * @return A reference to the vector of collision pairs associated with the specified collider entity. Each element in the vector represents an
             * index or identifier of another collider that is currently colliding or has recently collided with the specified collider. */
            [[nodiscard]] VE_FORCE_INLINE std::vector<i32> &GetCollisionPairs(Entity colliderEntity) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _collisionPairs[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Checks if the collision shape of the specified collider entity has changed size. This flag can be used to indicate that the collision
             * shape associated with the collider has undergone a size change, which may require updates to the physics simulation or broad-phase collision
             * detection. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider to be checked for collision shape size change.
             * @return True if the collision shape of the specified collider entity has changed size, false otherwise. */
            [[nodiscard]] VE_FORCE_INLINE bool HasCollisionShapeChangedSize(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return static_cast<bool>(_collisionShapeChangedSizeFlags[_entityToComponentIndex.find(colliderEntity)->second]);
            }

            /** @brief Sets whether the collision shape of the specified collider entity has changed size. This flag can be used to indicate that the collision
             * shape associated with the collider has undergone a size change, which may require updates to the physics simulation or broad-phase collision
             * detection. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose collision shape size change status is to be set.
             * @param hasChanged True to indicate that the collision shape has changed size, false otherwise. */
            VE_FORCE_INLINE void SetCollisionShapeChangedSize(Entity colliderEntity, bool hasChanged) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _collisionShapeChangedSizeFlags[_entityToComponentIndex.find(colliderEntity)->second] = static_cast<u8>(hasChanged);
            }

            /** @brief Checks if the specified collider entity is a trigger. A trigger is a special type of collider that does not participate in physics
             * simulation or collision response but can be used to detect overlaps and trigger events. The entity must have a ColliderComponent associated with
             * it.
             * @param colliderEntity The entity of the collider to be checked.
             * @return True if the specified collider entity is a trigger, false otherwise. */
            [[nodiscard]] VE_FORCE_INLINE bool IsTrigger(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return static_cast<bool>(_isTriggerFlags[_entityToComponentIndex.find(colliderEntity)->second]);
            }

            /** @brief Sets whether the specified collider entity is a trigger. A trigger is a special type of collider that does not participate in physics
             * simulation or collision response but can be used to detect overlaps and trigger events. The entity must have a ColliderComponent associated with
             * it.
             * @param colliderEntity The entity of the collider to be set as a trigger or not.
             * @param isTrigger True to set the collider as a trigger, false otherwise. */
            VE_FORCE_INLINE void SetTrigger(Entity colliderEntity, bool isTrigger) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _isTriggerFlags[_entityToComponentIndex.find(colliderEntity)->second] = static_cast<u8>(isTrigger);
            }

            /** @brief Checks if the specified collider entity is a simulation collider. A simulation collider participates in physics simulation and collision
             * response, while a non-simulation collider may be used for other purposes such as triggers or queries. The entity must have a ColliderComponent
             * associated with it.
             * @param colliderEntity The entity of the collider to be checked.
             * @return True if the specified collider entity is a simulation collider, false otherwise. */
            [[nodiscard]] VE_FORCE_INLINE bool IsSimulationCollider(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return static_cast<bool>(_isSimulationColliderFlags[_entityToComponentIndex.find(colliderEntity)->second]);
            }

            /** @brief Sets whether the specified collider entity is a simulation collider. A simulation collider participates in physics simulation and
             * collision response, while a non-simulation collider may be used for other purposes such as triggers or queries. The entity must have a
             * ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider to be set as a simulation collider or not.
             * @param isSimulationCollider True to set the collider as a simulation collider, false otherwise. */
            VE_FORCE_INLINE void SetSimulationCollider(Entity colliderEntity, bool isSimulationCollider) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _isSimulationColliderFlags[_entityToComponentIndex.find(colliderEntity)->second] = static_cast<u8>(isSimulationCollider);
            }

            /** @brief Checks if the specified collider entity is a query collider. A query collider is used for collision queries and does not participate in
             * physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider to be checked.
             * @return True if the specified collider entity is a query collider, false otherwise. */
            [[nodiscard]] VE_FORCE_INLINE bool IsQueryCollider(Entity colliderEntity) const {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return static_cast<bool>(_isQueryColliderFlags[_entityToComponentIndex.find(colliderEntity)->second]);
            }

            /** @brief Sets whether the specified collider entity is a query collider. A query collider is used for collision queries and does not participate
             * in physics simulation. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider to be set as a query collider or not.
             * @param isQueryCollider True to set the collider as a query collider, false otherwise. */
            VE_FORCE_INLINE void SetQueryCollider(Entity colliderEntity, bool isQueryCollider) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _isQueryColliderFlags[_entityToComponentIndex.find(colliderEntity)->second] = static_cast<u8>(isQueryCollider);
            }

            /** @brief Retrieves a reference to the Material associated with the specified collider entity. The entity must have a ColliderComponent associated
             * with it.
             * @param colliderEntity The entity whose material properties are to be retrieved.
             * @return A reference to the Material associated with the specified collider entity. */
            [[nodiscard]] VE_FORCE_INLINE Material &GetMaterial(Entity colliderEntity) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                return _materials[_entityToComponentIndex.find(colliderEntity)->second];
            }

            /** @brief Sets the material properties for the specified collider entity. The entity must have a ColliderComponent associated with it.
             * @param colliderEntity The entity of the collider whose material properties are to be set.
             * @param material The Material object containing the new material properties to be applied to the collider. */
            VE_FORCE_INLINE void SetMaterial(Entity colliderEntity, const Material &material) {
                VASSERT(HasComponent(colliderEntity), "Entity does not have a ColliderComponent.");

                _materials[_entityToComponentIndex.find(colliderEntity)->second] = material;
            }

            /** @brief Returns a contiguous view of the active local-to-world transforms.
             * @return A span over the densely packed active local-to-world TransformComponents at the front of the storage. */
            [[nodiscard]] VE_FORCE_INLINE std::span<const TransformComponent> GetActiveLocalToWorldTransforms() const {
                return { _localToWorldTransforms.data(), _activeCount };
            }

            /** @brief Returns a contiguous view of the active local-to-body transforms.
             * @return A span over the densely packed active local-to-body TransformComponents at the front of the storage. */
            [[nodiscard]] VE_FORCE_INLINE std::span<const TransformComponent> GetActiveLocalToBodyTransforms() const {
                return { _localToBodyTransforms.data(), _activeCount };
            }

            /** @brief Returns a contiguous view of the active broad-phase IDs.
             * @return A span over the densely packed active broad-phase IDs at the front of the storage. */
            [[nodiscard]] VE_FORCE_INLINE std::span<const i32> GetActiveBroadPhaseIDs() const {
                return { _broadPhaseIDs.data(), _activeCount };
            }

            /** @brief Returns a contiguous view of the active collision shape pointers.
             * @return A span over the densely packed active CollisionShape pointers at the front of the storage. */
            [[nodiscard]] VE_FORCE_INLINE std::span<CollisionShape *const> GetActiveCollisionShapes() const {
                return { _collisionShapes.data(), _activeCount };
            }

        protected:
            /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
             * @param indexA Index of the first component to swap.
             * @param indexB Index of the second component to swap. */
            void swapComponents(size_t indexA, size_t indexB) override;

            /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
            void removeLastComponentAndEntity() override;

        private:
            /** @brief Parallel array of body entities. Entry i is the rigid-body entity to which collider i is attached. */
            std::vector<Entity> _bodyEntities;

            /** @brief Parallel array of Collider pointers. Entry i points to the Collider object that owns collider i. */
            std::vector<Collider *> _colliders;

            /** @brief Parallel array of broad-phase IDs. Entry i holds the ID assigned by the broad-phase system, or -1 if not yet registered. */
            std::vector<i32> _broadPhaseIDs;

            /** @brief Parallel array of local-to-body transforms. Entry i converts from collider i's local space to its owning body's local space. */
            std::vector<TransformComponent> _localToBodyTransforms;

            /** @brief Parallel array of collision shape pointers. Entry i points to the geometric shape used for narrow-phase detection for collider i. */
            std::vector<CollisionShape *> _collisionShapes;

            /** @brief Parallel array of collision category bitmasks. Entry i defines the collision group collider i belongs to. */
            std::vector<u16> _collisionCategoryBits;

            /** @brief Parallel array of collides-with bitmasks. Entry i defines which collision categories collider i responds to. */
            std::vector<u16> _collidesWithMaskBits;

            /** @brief Parallel array of local-to-world transforms. Entry i converts from collider i's local space to world space. Updated each simulation
             * step by the physics system. */
            std::vector<TransformComponent> _localToWorldTransforms;

            /** @brief Parallel array of active collision pair lists. Entry i holds the broad-phase IDs of all colliders currently overlapping with collider i.
             * Pairs are added incrementally when overlap begins and removed when overlap ends; they are not rebuilt each frame. */
            std::vector<std::vector<i32>> _collisionPairs;

            /** @brief Parallel array of collision-shape-changed-size flags stored as u8 to keep the array tightly packed. A non-zero value for entry i
             * indicates the collision shape of collider i has been resized and the broad-phase bounding volume needs updating. */
            std::vector<u8> _collisionShapeChangedSizeFlags;

            /** @brief Parallel array of trigger flags stored as u8. A non-zero value for entry i indicates collider i is a trigger volume that generates
             * overlap events but does not participate in collision response. */
            std::vector<u8> _isTriggerFlags;

            /** @brief Parallel array of simulation-collider flags stored as u8. A non-zero value for entry i indicates collider i participates in physics
             * simulation and collision response. */
            std::vector<u8> _isSimulationColliderFlags;

            /** @brief Parallel array of query-collider flags stored as u8. A non-zero value for entry i indicates collider i is included in scene queries
             * (raycasts, shape casts, overlap tests) but does not participate in physics simulation. */
            std::vector<u8> _isQueryColliderFlags;

            /** @brief Parallel array of material properties. Entry i holds the friction, restitution, and other physical surface properties for collider i. */
            std::vector<Material> _materials;
    };

} // namespace Vulkyrie
