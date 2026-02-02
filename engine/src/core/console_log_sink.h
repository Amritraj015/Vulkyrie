#pragma once

#include "core/logger.h"

namespace Vulkyrie::Core {
    class ConsoleLogSink final : public LogSink {
        public:
            void LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) override;
    };
} // namespace Vulkyrie::Core
