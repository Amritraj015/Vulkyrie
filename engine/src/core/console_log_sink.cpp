#include "console_log_sink.h"
#include "defines.h"

namespace Vulkyrie::Core {
    void ConsoleLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        char buffer[LOG_BUFFER_SIZE];

        std::string_view logPrefix;

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

        auto it = std::copy(logPrefix.begin(), logPrefix.end(), buffer);
        auto result = std::vformat_to(it, fmt, args);

        FILE *out = (logLevel == LogLevel::Error || logLevel == LogLevel::Fatal) ? stderr : stdout;

        std::fwrite(buffer, 1, result - buffer, out);
        std::fputs("\033[0m\n", out);
        std::fflush(out);
    }
} // namespace Vulkyrie::Core
