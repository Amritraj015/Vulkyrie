#pragma once

namespace Vulkyrie::Core {
    enum class LogLevel {
        Fatal = 0, // Represents fatal errors that cause the application to terminate.
        Error,     // Represents error conditions that need attention.
        Warn,      // Represents warning conditions that are not critical.
        Info,      // Represents informational messages.
        Debug,     // Represents debug-level messages for development and troubleshooting.
        Trace,     // Represents trace-level messages for detailed debugging.
    };
}
