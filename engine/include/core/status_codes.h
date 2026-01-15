#pragma once

namespace Vulkyrie::Core {
    /// @brief This `enum` defines status codes to represent various operation results.
    enum class StatusCode : int {
        Successful = 0,                // Operation Successful.
        UnsupportedPlatform,           // Unsupported platform error.
        InvalidApplication,            // Invalid application error.
        PlatformAlreadyInitialized,    // Platform already initialized error.
        FailedToInitializeLogger,      // Failed to initialize logger error.
        UnsupportedLoggerType,         // Unsupported logger type error.
        FailedToCreateWindow,          // Failed to create window error.
        FailedToCreateGraphicsContext, // Failed to create graphics context error.
        FailedToInitializeGLAD,        // Failed to initialize GLAD error.
        FailedToCompileShaderProgram,  // Failed to create shader program error.
    };
} // namespace Vulkyrie::Core
