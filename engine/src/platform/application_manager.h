#pragma once

#include "core/status_codes.h"
#include "core/application.h"
#include "platform/window.h"

namespace Vulkyrie::Platform {
    class ApplicationManager {
        public:
            explicit ApplicationManager(const Vulkyrie::Core::Application &application);

            /** @brief Bootstraps a Vulkyrie engine application. */
            Vulkyrie::Core::StatusCode BootstrapApplication();

        private:
            const Vulkyrie::Core::Application &_application;

            /** @brief The main application window. */
            std::unique_ptr<Window> _window;

            Vulkyrie::Core::StatusCode InitializeSubSystems();
            Vulkyrie::Core::StatusCode TerminateSubSystems();
    };
}; // namespace Vulkyrie::Platform
