#pragma once

namespace Vulkyrie::Core {
    /// @brief This `enum` defines status codes to represent various operation results.
    enum class StatusCode : int {
        Successful = 0,             // Operation Successful.
        UnsupportedPlatform,        // Unsupported platform error.
        InvalidApplication,         // Invalid application error.
        PlatformAlreadyInitialized, // Platform already initialized error.
        FailedToInitializeLogger,   // Failed to initialize logger error.
        UnsupportedLoggerType,      // Unsupported logger type error.
        FailedToCreateWindow,       // Failed to create window error.
        FailedToInitializeGLAD,     // Failed to initialize GLAD error.
        // WaylandCannotConnectToDisplay,     // Linux (Wayland) connect to display.
        // WaylandCannotFindCompositor,       // Linux (Wayland) cannot find compositor.
        // WaylandCannotCreateSurface,        // Linux (Wayland) cannot create surface.
        // WaylandCannotCreateXdgSurface,     // Linux (Wayland) cannot create XDG surface.
        // WaylandCannotCreateTopLevelWindow, // Linux (Wayland) cannot create top level window.
    };
} // namespace Vulkyrie::Core
