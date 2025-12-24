#include "core/log_sink.h"

namespace Vulkyrie::Core {
    void LogSink::LogMessage(LogLevel logLevel, const char *fmt, va_list args) {
        char buffer[LOG_BUFFER_SIZE];

        const char *logPrefix;

        switch (logLevel) {
        case LogLevel::Fatal:
            logPrefix = "\033[41m[FATAL]: ";
            break;
        case LogLevel::Error:
            logPrefix = "\033[31m[ERROR]: ";
            break;
        case LogLevel::Warn:
            logPrefix = "\033[33m[WARN]: ";
            break;
        case LogLevel::Info:
            logPrefix = "\033[32m[INFO]: ";
            break;
        case LogLevel::Debug:
            logPrefix = "\033[34m[DEBUG]: ";
            break;
        case LogLevel::Trace:
            logPrefix = "\033[30m[TRACE]: ";
            break;
        }

        int offset = std::snprintf(buffer, LOG_BUFFER_SIZE, "%s", logPrefix);

        std::vsnprintf(buffer + offset, LOG_BUFFER_SIZE - offset, fmt, args);

        FILE *out = (logLevel == LogLevel::Error || logLevel == LogLevel::Fatal) ? stderr : stdout;

        std::fputs(buffer, out);
        std::fputs("\033[0m\n", out);
        std::fflush(out);
    }
} // namespace Vulkyrie::Core
