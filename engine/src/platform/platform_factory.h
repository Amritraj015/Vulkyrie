#pragma once

#include "defines.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
    #include "generic_platform.h"
    
    namespace Vulkyrie::Platform {
        Vulkyrie::Platform::PlatformBase* DetectPlatform() {
            static Vulkyrie::Platform::GenericPlatform platform;
            return &platform;
        }
    }
#else
    #include "platform/platform_base.h"
    
    namespace Vulkyrie::Platform {
        Vulkyrie::Platform::PlatformBase* DetectPlatform() {
            return nullptr;
        }
    }
#endif