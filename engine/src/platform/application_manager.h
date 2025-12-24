#pragma once

#include "core/status_codes.h"
#include "core/vulkyrie_application.h"
#include "platform_base.h"

namespace Vulkyrie::Platform {
    using Vulkyrie::Core::VulkyrieApplication;
    using Vulkyrie::Core::StatusCode;
    using Vulkyrie::Platform::PlatformBase;

    class ApplicationManager {
        public:
            explicit ApplicationManager(PlatformBase *platform, VulkyrieApplication *application);
            StatusCode BootstrapApplication();

        private:
            PlatformBase *_platform;
            VulkyrieApplication *_application;

            StatusCode InitializeSubSystems();
            StatusCode TerminateSubSystems();
    };
}; // namespace Engine
