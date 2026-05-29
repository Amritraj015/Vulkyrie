#include "core/console_log_sink.h"

namespace Vulkyrie {

    constexpr std::string_view consoleLogPrefixes[] = {
        "\033[41m[FATAL]: ", "\033[31m[ERROR]: ", "\033[33m[WARN]: ", "\033[32m[INFO]: ", "\033[34m[DEBUG]: ", "\033[90m[TRACE]: ",
    };

    void ConsoleLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        std::array<char, LOG_BUFFER_SIZE> buffer;

        const std::string_view prefix = consoleLogPrefixes[static_cast<size_t>(logLevel)];

        auto out = buffer.data();
        const auto end = buffer.data() + buffer.size();

        constexpr std::string_view reset = "\033[0m\n";

        // Copy prefix, reserving space for the reset sequence
        const size_t prefixSize = std::min(prefix.size(), buffer.size() > reset.size() ? buffer.size() - reset.size() : 0);

        std::memcpy(out, prefix.data(), prefixSize);
        out += prefixSize;

        // Format message safely, then copy what fits into the remaining buffer space
        const auto remaining = static_cast<size_t>(end - out);
        const size_t available = remaining > reset.size() ? remaining - reset.size() : 0;
        const std::string msg = std::vformat(fmt, args);
        const size_t copyLen = std::min(msg.size(), available);
        std::memcpy(out, msg.data(), copyLen);
        out += copyLen;

        // Append ANSI reset + newline
        std::memcpy(out, reset.data(), reset.size());
        out += reset.size();

        FILE *stream = (logLevel == LogLevel::Error || logLevel == LogLevel::Fatal) ? stderr : stdout;

        std::fwrite(buffer.data(), 1, static_cast<size_t>(out - buffer.data()), stream);
        std::fflush(stream);
    }

} // namespace Vulkyrie
