#pragma once

#include "core/status_codes.h"
#include "core/application.h"
#include "events/event.h"

namespace Vulkyrie::Platform {
    class Window {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Window(const Window &) = delete;
            void operator=(Window const &) = delete;

            virtual ~Window() = default;

            /** @brief Creates a new window for the application. */
            virtual Vulkyrie::Core::StatusCode Create() = 0;

            /** @brief Closes the application window. */
            virtual Vulkyrie::Core::StatusCode Close() = 0;

            /** @brief Reference to the application instance. */
            const Vulkyrie::Core::Application &appRef;

        protected:
            Window(const Vulkyrie::Core::Application &appRef) : appRef(appRef) { };
    };
} // namespace Vulkyrie::Platform
