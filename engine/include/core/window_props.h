#pragma once

#include "vlkypch.h"
#include "core/graphics_api.h"

namespace Vulkyrie {
    struct WindowProps final {
        public:
            /** @brief Starting height of the window. */
            u32 Height = 0;

            /** @brief Starting width of the window. */
            u32 Width = 0;

            /** @brief Title of the window. */
            std::string Title;

            /** @brief Whether to enable vertical synchronization (VSync) for the window. */
            bool EnableVSync = true;

            /** @brief The graphics API to use for rendering. */
            Vulkyrie::GraphicsAPI GraphicsAPI = GraphicsAPI::OpenGL;
    };
} // namespace Vulkyrie
