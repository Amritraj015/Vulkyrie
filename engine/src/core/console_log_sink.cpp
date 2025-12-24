#include <print>
#include "core/log_sink.h"

namespace Vulkyrie::Core {
    void LogSink::LogMessage(const char* message) {
        std::println("{}", message);
    }
} // namespace Vulkyrie::Core
