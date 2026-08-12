#include "core/ecs/entity.h"
#include "core/asserts.h"

namespace Vulkyrie {

    Entity::Entity(u64 index, u64 generation)
        : _id((index & ENTITY_INDEX_MASK) | ((generation & ENTITY_GENERATION_MASK) << ENTITY_INDEX_BITS)) {

        VASSERT(GetIndex() == index, "Entity index exceeds maximum allowed bits.");
        VASSERT(GetGeneration() == generation, "Entity generation exceeds maximum allowed bits.");
    }

} // namespace Vulkyrie
