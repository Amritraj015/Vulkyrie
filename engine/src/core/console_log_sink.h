#pragma once

#include "core/logger.h"

#include <mutex>

namespace Vulkyrie {

    class ConsoleLogSink final : public LogSink {
    public:
        void LogMessage(LogLevel logLevel, LogSite site, std::string_view fmt, std::format_args args) override;

    private:
        /** @brief Serializes the write+flush pair so messages from worker threads never interleave. */
        std::mutex _mutex;
    };

} // namespace Vulkyrie
