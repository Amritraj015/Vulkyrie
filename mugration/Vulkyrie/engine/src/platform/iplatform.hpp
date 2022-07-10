#pragma once

#include "defines.h"

class IPlatform
{
public:
    virtual void platform_console_write(const char *message, u8 color) = 0;
    virtual void platform_console_write_error(const char *message, u8 color) = 0;
};
