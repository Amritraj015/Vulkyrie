#pragma once

#include "core/graphics_api.h"
#include "core/types/static_string.h"

namespace Vulkyrie {

    struct WindowHandle final {
        WindowHandle(void *nativeWindow = nullptr, void *nativeDisplay = nullptr)
            : NativeWindow(nativeWindow)
            , NativeDisplay(nativeDisplay) {
        }

        void *NativeWindow;
        void *NativeDisplay;
    };

    struct WindowProps final {
    public:
        /** @brief Starting height of the window. */
        u32 Height = 0;

        /** @brief Starting width of the window. */
        u32 Width = 0;

        /** @brief Title of the window. */
        StaticString Title;

        /** @brief Whether to enable vertical synchronization (VSync) for the window. */
        bool EnableVSync = true;

        /** @brief The graphics API to use for rendering. */
        Vulkyrie::GraphicsAPI GraphicsAPI = GraphicsAPI::OpenGL;
    };

    struct ValidationSettings {
        std::unordered_set<std::string> Features;

        bool Has(std::string f) {
            if (auto it = Features.find(f); it != Features.end()) {
                return true;
            }

            return false;
        }
    };

    struct GraphicsSettings {
        ValidationSettings ValidationSettings{};
        GraphicsAPI API = GraphicsAPI::Vulkan;
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

    struct ApplicationInfo {
        StaticString Name;
        u32 MajorVersion;
        u32 MinorVersion;
        u32 PatchVersion;
    };

    struct ApplicationSettings {
        ApplicationInfo GeneralSettings;
        GraphicsSettings GraphicsSettings;
        AudioSettings AudioSettings;
        KeyboardSettings KeyboardSettings;
        MouseSettings MouseSettings;
    };

} // namespace Vulkyrie
