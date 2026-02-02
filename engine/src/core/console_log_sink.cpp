#include "core/console_log_sink.h"

namespace Vulkyrie::Core {
    static constexpr std::string_view consoleLogPrefixes[] = {
        "\033[41m[FATAL]: ", "\033[31m[ERROR]: ", "\033[33m[WARN]: ", "\033[32m[INFO]: ", "\033[34m[DEBUG]: ", "\033[90m[TRACE]: ",
    };

    void ConsoleLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        char buffer[LOG_BUFFER_SIZE];

        const std::string_view logPrefix = consoleLogPrefixes[static_cast<size_t>(logLevel)];

        const auto it = std::ranges::copy(logPrefix, buffer).out;
        const auto result = std::vformat_to(it, fmt, args);

        FILE *out = (logLevel == LogLevel::Error || logLevel == LogLevel::Fatal) ? stderr : stdout;

        std::fwrite(buffer, 1, result - buffer, out);
        std::fputs("\033[0m\n", out);
        std::fflush(out);
    }
} // namespace Vulkyrie::Core
