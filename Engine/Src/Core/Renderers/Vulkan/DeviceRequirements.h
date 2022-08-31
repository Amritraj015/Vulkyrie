#pragma once
#include "Defines.h"

namespace Vkr
{
    struct DeviceRequirements
    {
        bool graphics;
        bool present;
        bool compute;
        bool transfer;
        std::vector<const char *> deviceExtensionNames;
        bool samplerAnisotropy;
        bool discreteGpu;
    };
}