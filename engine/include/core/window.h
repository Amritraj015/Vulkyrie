#pragma once

#include "status_codes.h"
#include "application.h"

namespace Vulkyrie::Core {
    class Window {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Window(const Window &) = delete;
            void operator=(Window const &) = delete;

            virtual ~Window() = default;

            /** @brief Creates a new window for the application.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure.
             * */
            virtual Vulkyrie::Core::StatusCode Create() = 0;

            /** @brief Closes the application window.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure.
             * */
            virtual Vulkyrie::Core::StatusCode Close() = 0;

            /** @brief Toggles wireframe rendering mode.
             * @param enable True to enable wireframe mode, false to disable.
             */
            virtual void ToggleWireframeMode(bool enable) = 0;

            /** @brief Reference to the application instance. */
            const Vulkyrie::Core::Application &appRef;

        protected:
            Window(const Vulkyrie::Core::Application &appRef) : appRef(appRef) {};
    };
} // namespace Vulkyrie::Core
