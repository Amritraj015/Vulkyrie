#include "Logger.h"

namespace Vkr
{
    StatusCode Logger::InitializeLogging()
    {
        // TODO: create log file.
        VCREATE("Logger");

        return StatusCode::Successful;
    }

    StatusCode Logger::ShutdownLogging()
    {
        // TODO: cleanup logging/write queued entries.
        VDESTROY("Logger");

        return StatusCode::Successful;
    }

    void Logger::LogOutput(LogLevel level, const char *message, ...)
    {
        const char *level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
        bool is_error = level < LogLevel::Warn;

        // Technically imposes a 32k character limit on a single log entry, but...
        // DON'T DO THAT!
        const int message_length = 32000;
        char out_message[message_length];
        memset(out_message, 0, sizeof(out_message));

        // Format original message.
        // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
        // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
        // which is the type GCC/Clang's va_start expects.
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, message);
        vsnprintf(out_message, 32000, message, arg_ptr);
        va_end(arg_ptr);

        char out_message2[32000];
        const u8 levelIndex = to_underlying(level);
        sprintf(out_message2, "%s%s\n", level_strings[levelIndex], out_message);

        // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
        const char *color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};

        if (is_error)
        {
            printf("\033[%sm%s\033[0m", color_strings[levelIndex], out_message2);
        }
        else
        {
            printf("\033[%sm%s\033[0m", color_strings[levelIndex], out_message2);
        }
    }
}