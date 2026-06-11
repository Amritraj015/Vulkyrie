#pragma once

#include "physics/components/component_store.h"

namespace Vulkyrie {

    class Body;

    /** @brief Plain data bag that holds the pointer to a Body object. Passed by const reference to
     * BodyComponentStore::AddComponent so that callers provide a uniform construction interface
     * consistent with all other component stores. */
    struct BodyComponent final {
    public:
        /** @brief Non-owning pointer to the Body associated with the entity. The store does not
         * manage the lifetime of this object; the caller is responsible for ensuring the pointer
         * remains valid for as long as the component exists in the store. */
        Vulkyrie::Body *Body;
    };

    /** @brief Stores body components using a Structure-of-Arrays (SoA) layout.
     *
     * Each entity that participates in the physics simulation owns exactly one BodyComponent, which
     * binds it to a Body object and tracks the colliders attached to that body. The store maintains
     * the dense active-zone invariant inherited from ComponentStore: active components occupy indices
     * [0, _activeCount) and inactive components occupy [_activeCount, size). Swap operations keep
     * all parallel arrays (_bodies, _colliders, _bodyActiveFlags, _simulationColliderFlags) in sync
     * with _entities at all times. */
    class BodyComponentStore final : public ComponentStore {
    public:
        /** @brief Constructs an instance of BodyComponentStore. */
        BodyComponentStore();

        VE_DELETE_MOVE_AND_COPY(BodyComponentStore);

        /** @brief Destructor for BodyComponentStore. */
        ~BodyComponentStore() override = default;

        /** @brief Adds a BodyComponent to the specified entity. Active components are stored at the front of the
         * vector and inactive ones at the back to maintain dense packing for efficient iteration.
         * @param entity The entity to which the BodyComponent will be added. Must not already have a BodyComponent.
         * @param component The BodyComponent to be added to the entity.
         * @param active Whether the entity is currently active. */
        void AddComponent(Entity entity, const BodyComponent &component, bool active);

        /** @brief Appends the specified collider entity to the collider list of the given body entity. The body entity
         * must have a BodyComponent associated with it.
         * @param bodyEntity The entity that owns the body.
         * @param colliderEntity The collider entity to associate with this body. */
        VE_INLINE void AddColliderToBody(Entity bodyEntity, Entity colliderEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            _colliders[_entityToComponentIndex.find(bodyEntity)->second].push_back(colliderEntity);
        }

        /** @brief Removes the specified collider entity from the collider list of the given body entity. Uses
         * swap-erase so collider order within the list is not preserved. The body entity must have a BodyComponent
         * associated with it, and the collider entity must exist in the body's collider list.
         * @param bodyEntity The entity that owns the body.
         * @param colliderEntity The collider entity to remove. */
        VE_INLINE void RemoveColliderFromBody(Entity bodyEntity, Entity colliderEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            auto &colliders = _colliders[_entityToComponentIndex.find(bodyEntity)->second];
            auto it = std::find(colliders.begin(), colliders.end(), colliderEntity);

            if (it != colliders.end()) {
                *it = colliders.back();
                colliders.pop_back();
            } else {
                VASSERT(false, "Collider entity not found in body's collider list.");
            }
        }

        /** @brief Retrieves a reference to the Body associated with the specified body entity. The entity must have
         * a BodyComponent associated with it.
         * @param bodyEntity The entity whose body is to be retrieved.
         * @returns A reference to the Body associated with the specified entity. */
        [[nodiscard]] VE_INLINE Body &GetBody(Entity bodyEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            return *_bodies[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves a reference to the Body associated with the specified body entity. The entity must have
         * a BodyComponent associated with it.
         * @param bodyEntity The entity whose body is to be retrieved.
         * @returns A reference to the Body associated with the specified entity. */
        [[nodiscard]] VE_INLINE Body &GetBody(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            return *_bodies[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the list of collider entities attached to the specified body entity. The entity must
         * have a BodyComponent associated with it.
         * @param bodyEntity The entity whose collider list is to be retrieved.
         * @returns A const reference to the vector of collider entities attached to the specified body. */
        [[nodiscard]] VE_INLINE const std::vector<Entity> &GetColliders(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            return _colliders[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Checks whether the body associated with the specified entity is active in the physics simulation.
         * The entity must have a BodyComponent associated with it.
         * @param bodyEntity The entity to be checked.
         * @returns True if the body is active in the simulation, false otherwise. */
        [[nodiscard]] VE_INLINE bool IsBodyActive(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            return static_cast<bool>(_bodyActiveFlags[_entityToComponentIndex.find(bodyEntity)->second]);
        }

        /** @brief Sets whether the body associated with the specified entity is active in the physics simulation.
         * The entity must have a BodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param active True to mark the body as active in the simulation, false to deactivate it. */
        VE_INLINE void SetBodyActive(Entity bodyEntity, bool active) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            _bodyActiveFlags[_entityToComponentIndex.find(bodyEntity)->second] = static_cast<u8>(active);
        }

        /** @brief Checks whether the specified entity has any simulation colliders attached. A simulation collider
         * participates in collision response, as opposed to a query-only collider. The entity must have a
         * BodyComponent associated with it.
         * @param bodyEntity The entity to be checked.
         * @returns True if at least one simulation collider is attached, false otherwise. */
        [[nodiscard]] VE_INLINE bool HasSimulationColliders(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            return static_cast<bool>(_simulationColliderFlags[_entityToComponentIndex.find(bodyEntity)->second]);
        }

        /** @brief Sets whether the specified entity has simulation colliders attached. The entity must have a
         * BodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param hasSimulationColliders True if the body has at least one simulation collider, false otherwise. */
        VE_INLINE void SetHasSimulationColliders(Entity bodyEntity, bool hasSimulationColliders) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a BodyComponent.");

            _simulationColliderFlags[_entityToComponentIndex.find(bodyEntity)->second] = static_cast<u8>(hasSimulationColliders);
        }

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        /** @brief Parallel array of Body pointers. Entry i points to the Body object associated with entity i. */
        std::vector<Body *> _bodies;

        /** @brief Parallel array of collider entity lists. Entry i holds the collider entities attached to body i. */
        std::vector<std::vector<Entity>> _colliders;

        /** @brief Parallel array of physics-simulation active flags stored as u8. A non-zero value for entry i
         * indicates body i is active in the simulation. */
        std::vector<u8> _bodyActiveFlags;

        /** @brief Parallel array of simulation-collider flags stored as u8. A non-zero value for entry i indicates
         * body i has at least one simulation collider attached. */
        std::vector<u8> _simulationColliderFlags;
    };

} // namespace Vulkyrie
