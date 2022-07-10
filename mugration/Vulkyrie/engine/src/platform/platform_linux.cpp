#include "platform.hpp"
#include <iostream>

using namespace std;

// Linux platform layer.
#if V_PLATFORM_LINUX

void Platform::platform_console_write(const char *message, u8 color)
{
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char *color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

void Platform::platform_console_write_error(const char *message, u8 color)
{
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char *color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

#endif