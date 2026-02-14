#pragma once

namespace Vulkyrie::Renderer {

    template <typename T>
    concept has_preRead = requires(T a) {
        { a.preRead() } -> std::same_as<void>;
    };

    template <typename T>
    concept has_preWrite = requires(T a) {
        { a.preWrite() } -> std::same_as<void>;
    };

} // namespace Vulkyrie::Renderer
