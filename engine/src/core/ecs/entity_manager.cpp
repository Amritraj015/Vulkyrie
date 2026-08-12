#include "core/ecs/entity_manager.h"
#include "core/asserts.h"

namespace Vulkyrie {

    Entity EntityManager::CreateEntity() {
        size_t index;

        // If there are free indices available, we can reuse one of them.
        if (_freeIndices.size() > MINIMUM_FREE_INDICES) {
            // Get the index from the front of the deque of free indices and remove it from the deque.
            index = _freeIndices.front();
            _freeIndices.pop_front();
        } else {
            // The index for the new entity is the current size of the vector of generations, which represents the next available index.
            index = _generations.size();

            // If there are no free indices available,
            // we need to create a new one by adding a new generation entry to the vector of generations.
            _generations.push_back(0);

            // Assert that the index does not exceed the maximum allowed by the entity ID.
            VASSERT_EXPR(index < (u64(1) << Entity::ENTITY_INDEX_BITS), "Entity index exceeds maximum allowed bits.");
        }

        // Return a newly created entity.
        return Entity(static_cast<u64>(index), _generations[index]);
    }

    void EntityManager::DestroyEntity(const Entity entity) {
        // Get the index of the entity to be destroyed.
        const auto index = static_cast<size_t>(entity.GetIndex());

        // Increment the generation of the entity index to invalidate any existing entities with the same index and make it available for reuse.
        _generations[index]++;

        // Add the index of the destroyed entity to the deque of free indices for future reuse.
        _freeIndices.push_back(index);
    }

} // namespace Vulkyrie
