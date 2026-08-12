#pragma once

#include "core/graphics_api.h"
#include "core/types/static_string.h"

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

    struct GraphicsSettings {
        GraphicsAPI API = GraphicsAPI::OpenGL;
        u32 WindowHeight = 600;
        u32 WindowWidth = 800;
        bool EnableVSync = false;
    };

    struct AudioSettings {
        u8 Volume = 50;
    };

    struct KeyboardSettings {};

    struct MouseSettings {
        u8 SensitivityX;
        u8 SensitivityY;
    };

    struct ApplicationSettings {
        StaticString Name;
        GraphicsSettings GraphicsSettings;
        AudioSettings AudioSettings;
        KeyboardSettings KeyboardSettings;
        MouseSettings MouseSettings;
    };

} // namespace Vulkyrie
