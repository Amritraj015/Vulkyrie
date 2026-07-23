#include "core/console_log_sink.h"

#include "core/log_formatting.h"

namespace Vulkyrie {

    constexpr std::string_view consoleLogPrefixes[] = {
        "\033[41m[FATAL]: ", "\033[31m[ERROR]: ", "\033[33m[WARN]: ", "\033[32m[INFO]: ", "\033[34m[DEBUG]: ", "\033[90m[TRACE]: ",
    };

    void ConsoleLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        std::array<char, LOG_BUFFER_SIZE> buffer;

        const std::string_view prefix = consoleLogPrefixes[static_cast<size_t>(logLevel)];

        auto out = buffer.data();

        constexpr std::string_view reset = "\033[0m\n";
        static_assert(LOG_BUFFER_SIZE > reset.size(), "Log buffer must have room for the ANSI reset sequence.");

        // Copy prefix, reserving space for the reset sequence
        const size_t prefixSize = std::min(prefix.size(), buffer.size() - reset.size());

        std::memcpy(out, prefix.data(), prefixSize);
        out += prefixSize;

        // Format the message directly into the remaining buffer space (truncating if needed) so
        // logging never heap-allocates.
        const size_t available = buffer.size() - reset.size() - prefixSize;
        out += FormatToBuffer(out, available, fmt, args);

        // Append ANSI reset + newline
        std::memcpy(out, reset.data(), reset.size());
        out += reset.size();

        FILE *stream = (logLevel == LogLevel::Error || logLevel == LogLevel::Fatal) ? stderr : stdout;

        // Formatting above used only this call's stack buffer; the lock makes the write and the
        // flush atomic as a pair so concurrent worker-thread messages never interleave.
        const std::lock_guard<std::mutex> lock(_mutex);
        std::fwrite(buffer.data(), 1, static_cast<size_t>(out - buffer.data()), stream);
        std::fflush(stream);
    }

} // namespace Vulkyrie
