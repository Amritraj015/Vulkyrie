#pragma once

#include "core/status_codes.h"
#include "core/application.h"
#include "platform_base.h"

namespace Vulkyrie::Platform {
    class ApplicationManager {
        public:
            explicit ApplicationManager(Vulkyrie::Platform::PlatformBase *platform, Vulkyrie::Core::Application *application);
            Vulkyrie::Core::StatusCode BootstrapApplication();

        private:
            Vulkyrie::Platform::PlatformBase *_platform;
            Vulkyrie::Core::Application *_application;

            Vulkyrie::Core::StatusCode InitializeSubSystems();
            Vulkyrie::Core::StatusCode TerminateSubSystems();
    };
}; // namespace Vulkyrie::Platform
