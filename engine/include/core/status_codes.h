#pragma once

#include <cstdint>

#define RETURN_ON_FAILURE(expr)                                                                                                                                \
    do {                                                                                                                                                       \
        Vulkyrie::StatusCode _s = (expr);                                                                                                                      \
        if (Vulkyrie::StatusCode::Successful != _s) return _s;                                                                                                 \
    } while (false)

namespace Vulkyrie {
    /** @brief This `enum` defines status codes to represent various operation results. */
    enum class StatusCode : int32_t {
        Successful = 0,                    // Operation Successful.
        InvalidApplication,                // Invalid application error.
        FailedToInitializeLogger,          // Failed to initialize logger error.
        UnsupportedLoggerType,             // Unsupported logger type error.
        FailedToInitializeGLFW,            // Failed to initialize GLFW.
        FailedToCreateWindow,              // Failed to create window error.
        UnsupportedGraphicsAPI,            // Unsupported graphics API error.
        RendererAlreadyInitialized,        // Renderer already initialized.
        FailedToInitializeRendererContext, // Failed to initialize renderer context.
        FailedToInitializeGLAD,            // Failed to initialize GLAD error.
        FailedToCompileShaderProgram,      // Failed to create shader program error.
        JobSystemAlreadyInitialized,       // JobSystem::Initialize called while an explicit instance is already running.
        FailedToInitializeVolk,            // Failed to initialize Volk.
        FailedToCreateVulkanInstance,      // Failed to create vulkan instance.
        FailedToCreateSurface,             // Failed to create surface.
    };
} // namespace Vulkyrie
