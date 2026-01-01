#pragma once

#include "core/window_props.h"
#include "core/status_codes.h"
#include "events/event.h"

namespace Vulkyrie::Core {
    using EventCallbackFn = std::function<void(Vulkyrie::Events::Event &)>;

    class Window {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Window(const Window &) = delete;
            void operator=(Window const &) = delete;

            /** @brief Constructs a new Window with the given properties.
             * @param windowProps The properties for the window.
             * @param eventCallbackFn The callback function for handling events.
             */
            Window(const Vulkyrie::Core::WindowProps &windowProps, const EventCallbackFn &eventCallbackFn) noexcept
                : _windowProps(windowProps), _eventCallbackFn(eventCallbackFn) {
            }

            /** @brief Default constructor for the Window class. */
            virtual ~Window() = default;

            /** @brief Creates a new window for the application.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure.
             * */
            virtual Vulkyrie::Core::StatusCode Create() = 0;

            /** @brief Enables or disables vertical synchronization (VSync) for the window.
             * @param enabled True to enable VSync, false to disable.
             */
            virtual void SetVSync(bool enabled) = 0;

            /** @brief A callback that is called on each frame update cycle. */
            virtual inline void OnUpdate() const = 0;

            /** @brief Closes the application window.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure.
             * */
            virtual Vulkyrie::Core::StatusCode Close() = 0;

            /** @brief Toggles wireframe rendering mode.
             * @param enable True to enable wireframe mode, false to disable.
             */
            virtual void ToggleWireframeMode(bool enable) = 0;

        protected:
            const Vulkyrie::Core::WindowProps &_windowProps;
            EventCallbackFn _eventCallbackFn;
    };
} // namespace Vulkyrie::Core
