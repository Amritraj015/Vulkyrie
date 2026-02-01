#pragma once

#define VULKYRIE_FATAL_LEVEL_LOG 0
#define VULKYRIE_ERROR_LEVEL_LOG 1
#define VULKYRIE_WARN_LEVEL_LOG 2
#define VULKYRIE_INFO_LEVEL_LOG 3
#define VULKYRIE_DEBUG_LEVEL_LOG 4
#define VULKYRIE_TRACE_LEVEL_LOG 5

namespace Vulkyrie::Core {
    /** @brief This `enum` defines various log levels for logging messages. */
    enum class LogLevel {
        Fatal = 0, // Represents fatal errors that cause the application to terminate.
        Error = 1, // Represents error conditions that need attention.
        Warn = 2,  // Represents warning conditions that are not critical.
        Info = 3,  // Represents informational messages.
        Debug = 4, // Represents debug-level messages for development and troubleshooting.
        Trace = 5, // Represents trace-level messages for detailed debugging.
    };
} // namespace Vulkyrie::Core
