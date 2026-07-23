#include "core/file_log_sink.h"

#include "core/log_formatting.h"

namespace Vulkyrie {

    FileLogSink::~FileLogSink() {
        const std::lock_guard<std::mutex> lock(_mutex);
        if (nullptr != _logFile) {
            std::fflush(_logFile);
            std::fclose(_logFile);
            _logFile = nullptr;
        }
    }

    StatusCode FileLogSink::Initialize() {
        const std::lock_guard<std::mutex> lock(_mutex);
#if defined(VE_PLATFORM_WINDOWS)
        fopen_s(&_logFile, "vulkyrie_log.txt", "a");
#else
        _logFile = std::fopen("vulkyrie_log.txt", "a");
#endif

        if (nullptr == _logFile) {
            return StatusCode::FailedToInitializeLogger;
        }

        return StatusCode::Successful;
    }

    constexpr std::string_view fileLogPrefixes[] = { "[FATAL]:", "[ERROR]:", "[WARN]:", "[INFO]:", "[DEBUG]:", "[TRACE]:" };

    void FileLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        std::array<char, LOG_BUFFER_SIZE> buffer;

        const std::string_view prefix = fileLogPrefixes[static_cast<size_t>(logLevel)];

        char *out = buffer.data();
        char *end = buffer.data() + buffer.size();

        // Reserve space for newline
        const size_t writableSpace = buffer.size() > 1 ? buffer.size() - 1 : 0;

        // Copy prefix
        const size_t prefixSize = std::min(prefix.size(), writableSpace);

        std::memcpy(out, prefix.data(), prefixSize);
        out += prefixSize;

        // Append separator if possible
        if (static_cast<size_t>(end - out) > 1) {
            *out++ = ' ';
        }

        // Remaining writable space excluding newline
        const auto remaining = static_cast<size_t>(end - out);
        const size_t available = remaining > 1 ? remaining - 1 : 0;

        // Format the message directly into the remaining buffer space (truncating if needed) so
        // logging never heap-allocates.
        out += FormatToBuffer(out, available, fmt, args);

        // Append newline
        if (out < end) {
            *out++ = '\n';
        }

        // Formatting above used only this call's stack buffer; the lock guards `_logFile`'s
        // lifetime and keeps concurrent worker-thread messages from interleaving.
        const std::lock_guard<std::mutex> lock(_mutex);
        if (nullptr == _logFile) {
            return;
        }
        std::fwrite(buffer.data(), 1, static_cast<size_t>(out - buffer.data()), _logFile);
    }

} // namespace Vulkyrie
