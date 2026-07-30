#pragma once

#include "core/status_codes.h"
#include <string_view>
#include <format>
#include <memory>

// The following macro expansion values must align with the LogLevel enum values.
#define VULKYRIE_FATAL_LEVEL_LOG 0 // This is not required but added for completeness.
#define VULKYRIE_ERROR_LEVEL_LOG 1
#define VULKYRIE_WARN_LEVEL_LOG 2
#define VULKYRIE_INFO_LEVEL_LOG 3
#define VULKYRIE_DEBUG_LEVEL_LOG 4
#define VULKYRIE_TRACE_LEVEL_LOG 5

#if not defined(VULKYRIE_LOG_LEVEL)
#define VULKYRIE_LOG_LEVEL VULKYRIE_ERROR_LEVEL_LOG
#endif

namespace Vulkyrie {

    enum class LoggerType { Console, File };

    /** @brief Strips the directory from a path, leaving just the file name.
     *
     * `consteval` so the scan happens entirely at compile time: `__FILE__` expands to whatever path the build
     * system handed the compiler, which for this project is absolute, and no log call should pay to walk it. */
    [[nodiscard]] consteval std::string_view FileNameFromPath(std::string_view path) {
        const std::size_t separator = path.find_last_of("/\\");

        return separator == std::string_view::npos ? path : path.substr(separator + 1);
    }

    /** @brief Where a log call was written. Captured by the logging macros so a message can name its origin
     * without the caller repeating it in every format string. */
    struct LogSite {
    public:
        /** @brief File name only, with the directory already stripped. */
        std::string_view FileName;

        /** @brief Line the log call sits on. */
        int Line = 0;
    };

    /** @brief This `enum` defines various log levels for logging messages. */
    enum class LogLevel {
        Fatal = VULKYRIE_FATAL_LEVEL_LOG, // Represents fatal errors that cause the application to terminate.
        Error = VULKYRIE_ERROR_LEVEL_LOG, // Represents error conditions that need attention.
        Warn = VULKYRIE_WARN_LEVEL_LOG,   // Represents warning conditions that are not critical.
        Info = VULKYRIE_INFO_LEVEL_LOG,   // Represents informational messages.
        Debug = VULKYRIE_DEBUG_LEVEL_LOG, // Represents debug-level messages for development and troubleshooting.
        Trace = VULKYRIE_TRACE_LEVEL_LOG, // Represents trace-level messages for detailed debugging.
    };

    class LogSink {
    public:
        virtual ~LogSink() = default;

        /** Initializes the logger. */
        virtual StatusCode Initialize() {
            return StatusCode::Successful;
        }

        virtual void LogMessage(LogLevel logLevel, LogSite site, std::string_view fmt, std::format_args args) = 0;

    protected:
        static constexpr unsigned short LOG_BUFFER_SIZE = 2048;
    };

    class Logger {
    public:
        // Deleted copy constructor and assignment operator to prevent copies.
        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;

        Logger(Logger &&) = delete;
        Logger &operator=(Logger &&) = delete;

        static StatusCode InitializeLogger(LoggerType loggerType);

        template <typename... Args> static void Log(LogLevel logLevel, LogSite site, std::string_view fmt, Args &&...args) {
            if (nullptr == _logSink) return;
            _logSink->LogMessage(logLevel, site, fmt, std::make_format_args(args...));
        }

    private:
        static std::unique_ptr<LogSink> _logSink;
    };
} // namespace Vulkyrie

// clang-format off

// Captures the file and line of the macro's expansion point. Kept as its own macro so every level agrees.
#define VE_LOG_SITE LogSite{ FileNameFromPath(__FILE__), __LINE__ }

// Logs a fatal message.
#define VFATAL(fmt, ...) Logger::Log(LogLevel::Fatal, VE_LOG_SITE, fmt __VA_OPT__(,) __VA_ARGS__)

// Logs an error message.
#define VERROR(fmt, ...) Logger::Log(LogLevel::Error, VE_LOG_SITE, fmt __VA_OPT__(,) __VA_ARGS__)

// Logs a warning message if warning level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_WARN_LEVEL_LOG
#define VWARN(fmt, ...) Logger::Log(LogLevel::Warn, VE_LOG_SITE, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define VWARN(fmt, ...)
#endif

// Logs an info message if info level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_INFO_LEVEL_LOG
#define VINFO(fmt, ...) Logger::Log(LogLevel::Info, VE_LOG_SITE, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define VINFO(fmt, ...)
#endif

// Logs a debug message if debug level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_DEBUG_LEVEL_LOG
#define VDEBUG(fmt, ...) Logger::Log(LogLevel::Debug, VE_LOG_SITE, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define VDEBUG(fmt, ...)
#endif

// Logs a trace message if trace level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_TRACE_LEVEL_LOG
#define VTRACE(fmt, ...) Logger::Log(LogLevel::Trace, VE_LOG_SITE, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define VTRACE(fmt, ...)
#endif

// clang-format on
