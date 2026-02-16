#pragma once

#include <concepts>

namespace Vulkyrie::Renderer {

    /** @brief Concept that defines the requirements for a frame graph resource backend. */
    template <typename T>
    concept FrameGraphResourceBackend = std::is_default_constructible_v<T> && std::is_move_constructible_v<T> && requires(T a) {
        typename T::Descriptor;
        { a.Create(std::declval<const typename T::Descriptor &>(), nullptr) } -> std::same_as<void>;
        { a.Destroy(std::declval<const typename T::Descriptor &>(), nullptr) } -> std::same_as<void>;
    };

    /** @brief Concept that checks if a type has a preRead method. */
    template <typename T>
    concept has_preRead = requires(T a) {
        { a.preRead() } -> std::same_as<void>;
    };

    /** @brief Concept that checks if a type has a preWrite method. */
    template <typename T>
    concept has_preWrite = requires(T a) {
        { a.preWrite() } -> std::same_as<void>;
    };

} // namespace Vulkyrie::Renderer
