#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief The Entity struct represents a unique identifier for an entity in the ECS architecture. It encodes both an index and a generation to allow for
     * efficient reuse of entity IDs while preventing issues with dangling references. The index is used to locate the entity's components in the storage, while
     * the generation is used to distinguish between different incarnations of the same index after an entity has been destroyed and its index reused. */
    struct Entity {
    public:
        static constexpr u64 ENTITY_INDEX_BITS = 48;
        static constexpr u64 ENTITY_INDEX_MASK = (1ULL << ENTITY_INDEX_BITS) - 1;
        static constexpr u64 ENTITY_GENERATION_BITS = 16;
        static constexpr u64 ENTITY_GENERATION_MASK = (1ULL << ENTITY_GENERATION_BITS) - 1;

        /** @brief Constructs an entity with the given index and generation.
         * @param index The index of the entity in the entity manager's storage.
         * @param generation The generation of the entity used to distinguish between different incarnations of the same index.
         */
        Entity(u64 index, u64 generation);

        /** @brief Gets the unique identifier of the entity.
         * @returns The unique identifier of the entity.
         */
        [[nodiscard]] VE_INLINE u64 GetID() const {
            return _id;
        }

        /** @brief Gets the index of the entity.
         * @returns The index of the entity.
         */
        [[nodiscard]] VE_INLINE u64 GetIndex() const {
            return _id & ENTITY_INDEX_MASK;
        }

        /** @brief Gets the generation of the entity.
         * @returns The generation of the entity.
         */
        [[nodiscard]] VE_INLINE u64 GetGeneration() const {
            return (_id >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
        }

        /** @brief Equality operator for comparing two entities.
         * @param other The other entity to compare with.
         * @returns True if the entities are equal, false otherwise.
         */
        [[nodiscard]] VE_INLINE bool operator==(const Entity &other) const {
            return _id == other._id;
        }

        /** @brief Inequality operator for comparing two entities.
         * @param other The other entity to compare with.
         * @returns True if the entities are not equal, false otherwise.
         */
        [[nodiscard]] VE_INLINE bool operator!=(const Entity &other) const {
            return _id != other._id;
        }

    private:
        /** @brief Unique identifier for the entity. */
        u64 _id;
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
