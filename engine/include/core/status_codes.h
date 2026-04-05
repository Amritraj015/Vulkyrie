#pragma once

#include <cstdint>

#define RETURN_ON_FAILURE(expr)                                                                                                                                \
    do {                                                                                                                                                       \
        StatusCode _s = (expr);                                                                                                                \
        if (_s != StatusCode::Successful) return _s;                                                                                           \
    } while (false)

namespace Vulkyrie {
    /** @brief This `enum` defines status codes to represent various operation results. */
    enum class StatusCode : int32_t {
        Successful = 0,               // Operation Successful.
        InvalidApplication,           // Invalid application error.
        FailedToInitializeLogger,     // Failed to initialize logger error.
        UnsupportedLoggerType,        // Unsupported logger type error.
        FailedToCreateWindow,         // Failed to create window error.
        UnsupportedGraphicsAPI,       // Unsupported graphics API error.
        FailedToInitializeGLAD,       // Failed to initialize GLAD error.
        FailedToCompileShaderProgram, // Failed to create shader program error.
    };
} // namespace Vulkyrie
