#pragma once

#include "vlkypch.h"
#include "core/asserts.h"

namespace Vulkyrie {
    /** @brief The FrameGraphBlackboard class provides a type-safe storage mechanism for arbitrary data associated with a frame graph.
     * It allows passes to store and retrieve data of any type without needing to define a specific structure for the blackboard.
     */
    class FrameGraphBlackboard final {
        public:
            FrameGraphBlackboard() = default;
            ~FrameGraphBlackboard() = default;

            FrameGraphBlackboard(const FrameGraphBlackboard &) = default;
            FrameGraphBlackboard &operator=(const FrameGraphBlackboard &) = default;

            FrameGraphBlackboard(FrameGraphBlackboard &&) = default;
            FrameGraphBlackboard &operator=(FrameGraphBlackboard &&) = default;

            /** @brief Stores a value of type T in the blackboard. The value is constructed in-place using the provided arguments.
             * @tparam T The type of the value to store. Must not already exist in the blackboard.
             * @tparam Args The types of the arguments to forward to the constructor of T.
             * @param args The arguments to forward to the constructor of T.
             * @return A reference to the stored value of type T. */
            template <typename T, typename... Args> T &Set(Args &&...args) {
                VASSERT_EXPR(!Contains<T>(), "Blackboard already contains an entry for this type.");

                return std::any_cast<T &>(_cache[typeid(T)] = T(std::forward<Args>(args)...));
            }

            /** @brief Retrieves a reference to the value of type T stored in the blackboard.
             * @tparam T The type of the value to retrieve. Must exist in the blackboard.
             * @return A reference to the stored value of type T. */
            template <typename T> [[nodiscard]] T &Get() {
                return const_cast<T &>(const_cast<const FrameGraphBlackboard *>(this)->Get<T>());
            }

            /** @brief Retrieves a pointer to the value of type T stored in the blackboard, or nullptr if it does not exist.
             * @tparam T The type of the value to retrieve.
             * @return A pointer to the stored value of type T, or nullptr if it does not exist. */
            template <typename T> [[nodiscard]] T *TryGet() {
                return const_cast<T *>(const_cast<const FrameGraphBlackboard *>(this)->TryGet<T>());
            }

            /** @brief Retrieves a reference to the value of type T stored in the blackboard.
             * @tparam T The type of the value to retrieve. Must exist in the blackboard.
             * @return A reference to the stored value of type T. */
            template <typename T> [[nodiscard]] const T &Get() const {
                VASSERT_EXPR(Contains<T>(), "Blackboard does not contain an entry for this type.");

                return std::any_cast<const T &>(_cache.at(typeid(T)));
            }

            /** @brief Retrieves a pointer to the value of type T stored in the blackboard, or nullptr if it does not exist.
             * @tparam T The type of the value to retrieve.
             * @return A pointer to the stored value of type T, or nullptr if it does not exist. */
            template <typename T> [[nodiscard]] const T *TryGet() const {
                auto it = _cache.find(typeid(T));

                return it != _cache.end() ? std::any_cast<const T>(&it->second) : nullptr;
            }

            /** @brief Checks if the blackboard contains a value of type T.
             * @tparam T The type of the value to check for.
             * @return true if the blackboard contains a value of type T, false otherwise. */
            template <typename T> [[nodiscard]] bool Contains() const {
                return _cache.contains(typeid(T));
            }

        private:
            /** @brief The underlying storage for the blackboard, mapping type indices to any values. */
            std::unordered_map<std::type_index, std::any> _cache;
    };
} // namespace Vulkyrie
