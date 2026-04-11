#pragma once

#include "vlkypch.h"
#include "core/asserts.h"

namespace Vulkyrie {

    class EntityManager;

    struct Entity {
        public:
            /** @brief Gets the unique identifier of the entity.
             * @return The unique identifier of the entity.
             */
            [[nodiscard]] VE_FORCE_INLINE u64 GetID() const {
                return _id;
            }

            /** @brief Gets the index of the entity.
             * @return The index of the entity.
             */
            [[nodiscard]] VE_FORCE_INLINE u64 GetIndex() const {
                return _id & ENTITY_INDEX_MASK;
            }

            /** @brief Gets the generation of the entity.
             * @return The generation of the entity.
             */
            [[nodiscard]] VE_FORCE_INLINE u64 GetGeneration() const {
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
            u64 _id;

            static constexpr u64 ENTITY_INDEX_BITS = 48;
            static constexpr u64 ENTITY_INDEX_MASK = (1ULL << ENTITY_INDEX_BITS) - 1;
            static constexpr u64 ENTITY_GENERATION_BITS = 16;
            static constexpr u64 ENTITY_GENERATION_MASK = (1ULL << ENTITY_GENERATION_BITS) - 1;
            // static constexpr u32 MINIMUM_FREE_INDICES = 2048;

            /** @brief Constructs an entity with the given index and generation.
             * @param index The index of the entity in the entity manager's storage.
             * @param generation The generation of the entity used to distinguish between different incarnations of the same index.
             */
            Entity(u64 index, u64 generation)
                : _id((index & ENTITY_INDEX_MASK) | ((generation & ENTITY_GENERATION_MASK) << ENTITY_INDEX_BITS)) {
                VASSERT_EXPR(index < (1ULL << ENTITY_INDEX_BITS), "Entity index exceeds maximum allowed bits.");
                VASSERT_EXPR(generation < (1ULL << ENTITY_GENERATION_BITS), "Entity generation exceeds maximum allowed bits.");
            }

            friend class EntityManager;
    };

} // namespace Vulkyrie

namespace std {
    // Hash function for an Entity
    template <> struct hash<Vulkyrie::Entity> {
        public:
            size_t operator()(const Vulkyrie::Entity &entity) const noexcept {
                return static_cast<size_t>(entity.GetID());
            }
    };
} // namespace std
