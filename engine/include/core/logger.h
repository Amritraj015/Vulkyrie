#pragma once

#include "defines.h"
#include "status_codes.h"
#include "logger_type.h"
#include "log_sink.h"

// Enable log levels based on build configuration.
// FATAL and ERROR level logs are always enabled.
#if defined(VULKYRIE_DEBUG)
    #define V_LOG_WARN_ENABLED
    #define V_LOG_INFO_ENABLED
    #define V_LOG_DEBUG_ENABLED
    #define V_LOG_TRACE_ENABLED
#endif

namespace Vulkyrie::Core {
    class Logger {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Logger(const Logger &) = delete;
            Logger &operator=(const Logger &) = delete;

            static StatusCode InitializeLogger(LoggerType loggerType);

            static void Log(const char *message);

            static StatusCode TerminateLogger();

        private:
            static std::unique_ptr<LogSink> _logSink;
    };
}


// Logs a fatal message.
#define VFATAL(message, ...) Vulkyrie::Core::Logger::Log(std::format("\033[41m[FATAL] {}\033[0m", std::format(message, ##__VA_ARGS__)).c_str());
#define VERROR(message, ...) Vulkyrie::Core::Logger::Log(std::format("\033[31m[ERROR] {}\033[0m", std::format(message, ##__VA_ARGS__)).c_str());

#if defined(V_LOG_WARN_ENABLED)
    #define VWARN(message, ...) Vulkyrie::Core::Logger::Log(std::format("\033[33m[WARN] {}\033[0m", std::format(message, ##__VA_ARGS__)).c_str());
#else
    #define VWARN(message, ...)
#endif

#if defined(V_LOG_INFO_ENABLED)
    #define VINFO(message, ...) Vulkyrie::Core::Logger::Log(std::format("\033[32m[INFO] {}\033[0m", std::format(message, ##__VA_ARGS__)).c_str());
#else
    #define VINFO(message, ...)
#endif

#if defined(V_LOG_DEBUG_ENABLED)
    #define VDEBUG(message, ...) Vulkyrie::Core::Logger::Log(std::format("\033[34m[DEBUG] {}\033[0m", std::format(message, ##__VA_ARGS__)).c_str());
#else
    #define VDEBUG(message, ...)
#endif

#if defined(V_LOG_TRACE_ENABLED)
    #define VTRACE(message, ...) Vulkyrie::Core::Logger::Log(std::format("\033[00m[TRACE] {}\033[0m", std::format(message, ##__VA_ARGS__)).c_str());
#else
    #define VTRACE(message, ...)
#endif