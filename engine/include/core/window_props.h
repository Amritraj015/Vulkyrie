#pragma once

#include "vlkypch.h"
#include "core/graphics_api.h"

namespace Vulkyrie::Core {
    struct WindowProps {
        public:
            /** @brief Starting position of the window on x-axis. */
            u32 StartX = 0;

            /** @brief Starting position of the window on y-axis. */
            u32 StartY = 0;

            /** @brief Starting height of the window. */
            u32 Height = 0;

            /** @brief Starting width of the window. */
            u32 Width = 0;

            /** @brief Title of the window. */
            std::string Title;

            /** @brief Enable or disable VSync. */
            bool VSync = true;

            /** @brief The graphics API to use for rendering. */
            Vulkyrie::Core::GraphicsAPI GraphicsApi = Vulkyrie::Core::GraphicsAPI::OpenGL;
    };
} // namespace Vulkyrie::Core
