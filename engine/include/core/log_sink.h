#pragma once

#include "status_codes.h"
#include "core/log_level.h"
#include "vlkypch.h"

namespace Vulkyrie::Core {
    class LogSink {
        public:
            virtual ~LogSink() = default;

            /** Initializes the logger. */
            virtual StatusCode Initialize() {
                return StatusCode::Successful;
            }

            virtual void LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) = 0;

        protected:
            static constexpr u16 LOG_BUFFER_SIZE = 512;
    };
} // namespace Vulkyrie::Core
