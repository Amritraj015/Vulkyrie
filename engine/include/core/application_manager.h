#pragma once

#include "core/status_codes.h"
#include "core/application.h"
#include "core/window.h"

namespace Vulkyrie::Core {
    class ApplicationManager {
        public:
            explicit ApplicationManager(const Vulkyrie::Core::Application &application);

            /** @brief Bootstraps a Vulkyrie engine application. */
            Vulkyrie::Core::StatusCode BootstrapApplication();

            /** @brief Toggles wireframe rendering mode.
             * @param enable True to enable wireframe mode, false to disable.
             */
            static void ToggleWireframeMode(bool enable);

        private:
            static ApplicationManager *_instance;
            const Vulkyrie::Core::Application &_application;

            /** @brief The main application window. */
            std::shared_ptr<Window> _window;

            Vulkyrie::Core::StatusCode InitializeSubSystems();
            Vulkyrie::Core::StatusCode TerminateSubSystems();
    };
}; // namespace Vulkyrie::Core
