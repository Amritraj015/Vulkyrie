#pragma once

#include "core/logger.h"

#include <cstdio>
#include <mutex>

namespace Vulkyrie {
    class FileLogSink final : public LogSink {
    public:
        StatusCode Initialize() override;
        void LogMessage(LogLevel logLevel, LogSite site, std::string_view fmt, std::format_args args) override;
        ~FileLogSink() override;

    private:
        /** @brief Guards `_logFile`'s lifetime and serializes writes from worker threads. */
        std::mutex _mutex;
        FILE *_logFile;
    };
} // namespace Vulkyrie
