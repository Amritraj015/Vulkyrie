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

    struct Extent2D final {
        u32 Width = 0;
        u32 Height = 0;

        Extent2D() = default;

        Extent2D(u32 width, u32 height)
            : Width(width)
            , Height(height) {
        }
    };

    struct WindowProps final {
    public:
        /** @brief The dimensions for the window to be created. */
        Extent2D Dimensions;

        /** @brief Title of the window. */
        StaticString Title;

        /** @brief Whether to enable vertical synchronization (VSync) for the window. */
        bool EnableVSync = true;

        /** @brief The graphics API to use for rendering. */
        Vulkyrie::GraphicsAPI GraphicsAPI = GraphicsAPI::Vulkan;
    };

    struct ValidationSettings {
        std::unordered_set<std::string> Features;

        bool Has(std::string &f) const {
            return Features.contains(f);
        }
    };

    struct GraphicsSettings {
        ValidationSettings ValidationSettings{};
        Extent2D WindowDimensions{ 800, 600 };
        GraphicsAPI API = GraphicsAPI::Vulkan;
        bool EnableVSync = false;
    };

    struct AudioSettings {
        u8 Volume = 50;
    };

    struct KeyboardSettings {};

    enum class MouseSensitivity : u8 { One = 1, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten };

    struct MouseSettings {
        MouseSensitivity SensitivityX = MouseSensitivity::One;
        MouseSensitivity SensitivityY = MouseSensitivity::One;
    };

    struct Version final {
        u32 Major = 0;
        u32 Minor = 0;
        u32 Patch = 1;

        constexpr Version() noexcept = default;

        constexpr Version(u32 major, u32 minor, u32 patch) noexcept
            : Major(major)
            , Minor(minor)
            , Patch(patch) {
        }

        std::string ToString() const {
            return std::format("v{}.{}.{}", Major, Minor, Patch);
        }
    };

    struct ApplicationInfo {
        StaticString Name;
        Version Version;
    };

    struct ApplicationSettings {
        ApplicationInfo GeneralSettings;
        GraphicsSettings GraphicsSettings;
        AudioSettings AudioSettings;
        KeyboardSettings KeyboardSettings;
        MouseSettings MouseSettings;
    };

} // namespace Vulkyrie
