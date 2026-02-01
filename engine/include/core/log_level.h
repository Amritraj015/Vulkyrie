#pragma once

// The following macro expansion values must align with the LogLevel enum values.
#define VULKYRIE_FATAL_LEVEL_LOG 0 // This is not required but added for completeness.
#define VULKYRIE_ERROR_LEVEL_LOG 1
#define VULKYRIE_WARN_LEVEL_LOG 2
#define VULKYRIE_INFO_LEVEL_LOG 3
#define VULKYRIE_DEBUG_LEVEL_LOG 4
#define VULKYRIE_TRACE_LEVEL_LOG 5

namespace Vulkyrie::Core {
    /** @brief This `enum` defines various log levels for logging messages. */
    enum class LogLevel {
        Fatal = VULKYRIE_FATAL_LEVEL_LOG, // Represents fatal errors that cause the application to terminate.
        Error = VULKYRIE_ERROR_LEVEL_LOG, // Represents error conditions that need attention.
        Warn = VULKYRIE_WARN_LEVEL_LOG,   // Represents warning conditions that are not critical.
        Info = VULKYRIE_INFO_LEVEL_LOG,   // Represents informational messages.
        Debug = VULKYRIE_DEBUG_LEVEL_LOG, // Represents debug-level messages for development and troubleshooting.
        Trace = VULKYRIE_TRACE_LEVEL_LOG, // Represents trace-level messages for detailed debugging.
    };
} // namespace Vulkyrie::Core
