#pragma once

#include "vlkypch.h"

namespace Vulkyrie::ECS {

    class Entity {
        public:
            /** @brief Gets the unique identifier of the entity.
             * @return The unique identifier of the entity.
             */
            [[nodiscard]] VE_FORCE_INLINE u32 GetID() const {
                return _id;
            }

            /** @brief Gets the index of the entity.
             * @return The index of the entity.
             */
            [[nodiscard]] VE_FORCE_INLINE u32 GetIndex() const {
                return _id & ENTITY_INDEX_MASK;
            }

            /** @brief Gets the generation of the entity.
             * @return The generation of the entity.
             */
            [[nodiscard]] VE_FORCE_INLINE u32 GetGeneration() const {
                return (_id >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
            }

            /** @brief Equality operator for comparing two entities.
             * @param other The other entity to compare with.
             * @return True if the entities are equal, false otherwise.
             */
            [[nodiscard]] VE_FORCE_INLINE bool operator==(const Entity &other) const {
                return _id == other._id;
            }

            /** @brief Inequality operator for comparing two entities.
             * @param other The other entity to compare with.
             * @return True if the entities are not equal, false otherwise.
             */
            [[nodiscard]] VE_FORCE_INLINE bool operator!=(const Entity &other) const {
                return _id != other._id;
            }

        private:
            /** @brief Unique identifier for the entity. */
            u32 _id = 0;

            static constexpr u32 ENTITY_INDEX_BITS = 24;
            static constexpr u32 ENTITY_INDEX_MASK = (1 << ENTITY_INDEX_BITS) - 1;
            static constexpr u32 ENTITY_GENERATION_BITS = 8;
            static constexpr u32 ENTITY_GENERATION_MASK = (1 << ENTITY_GENERATION_BITS) - 1;
            // static constexpr u32 MINIMUM_FREE_INDICES = 1024;

        public:
            /** @brief Constructs an entity with the given index and generation.
             * @param index The index of the entity in the entity manager's storage.
             * @param generation The generation of the entity used to distinguish between different incarnations of the same index.
             */
            Entity(u32 index, u32 generation)
                : _id((index & ENTITY_INDEX_MASK) | ((generation & ENTITY_GENERATION_MASK) << ENTITY_INDEX_BITS)) {
            }
    };

} // namespace Vulkyrie::ECS
