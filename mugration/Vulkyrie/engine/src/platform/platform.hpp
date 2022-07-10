#pragma once

#include "defines.h"

#include "iplatform.hpp"

class Platform : public IPlatform
{
    virtual void platform_console_write(const char *message, u8 color) override;
    virtual void platform_console_write_error(const char *message, u8 color) override;
};