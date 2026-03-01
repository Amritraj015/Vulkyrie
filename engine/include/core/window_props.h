#pragma once

#include "vlkypch.h"
#include "core/graphics_api.h"

namespace Vulkyrie::Core {
    struct WindowProps final {
        public:
            /** @brief Starting height of the window. */
            u32 Height = 0;

            /** @brief Starting width of the window. */
            u32 Width = 0;

            /** @brief Title of the window. */
            std::string Title;

            /** @brief The graphics API to use for rendering. */
            GraphicsAPI GraphicsAPI = GraphicsAPI::OpenGL;
    };
} // namespace Vulkyrie::Core
