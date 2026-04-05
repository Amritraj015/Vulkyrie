#pragma once

#include "core/window_props.h"
#include "core/status_codes.h"
#include "events/event.h"

namespace Vulkyrie {
    using EventCallbackFn = std::function<void(Event &)>;

    class Platform {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Platform(const Platform &) = delete;
            Platform &operator=(const Platform &) = delete;

            Platform(Platform &&) = delete;
            Platform &operator=(Platform &&) = delete;

            /** @brief Constructs a new Window with the given properties.
             * @param windowProps The properties for the window.
             * @param eventCallbackFn The callback function for handling events.
             */
            Platform(const WindowProps &windowProps, const EventCallbackFn &eventCallbackFn) noexcept
                : _windowProps(windowProps)
                , _eventCallbackFn(eventCallbackFn) {
            }

            /** @brief Default constructor for the Window class. */
            virtual ~Platform() = default;

            /** @brief Gets the native window handle.
             * @returns A pointer to the native window.
             */
            [[nodiscard]] inline virtual void *GetWindowHandle() const = 0;

            /** @brief Creates a new window for the application.
             * @returns StatusCode indicating success or failure.
             * */
            [[nodiscard]] virtual StatusCode CreateWindow() = 0;

            /** @brief Enables or disables vertical synchronization (VSync) for the window.
             * @param enable True to enable VSync, false to disable.
             */
            virtual inline void SetVSync(bool enable) = 0;

            /** @brief A callback that is called on each frame update cycle. */
            virtual inline void OnUpdate() const = 0;

            /** @brief Closes the application window.
             * @returns StatusCode indicating success or failure.
             * */
            virtual StatusCode CloseWindow() = 0;

            /** @brief Gets the current time in seconds since the window was created.
             * @returns The current time in seconds.
             */
            [[nodiscard]] virtual inline f32 GetTime() const = 0;

            /** @brief Captures or releases the mouse cursor when the window gains or loses focus.
             * @param enable True to capture the mouse on focus, false to release it.
             */
            virtual inline void CaptureMouseOnFocus(bool enable) = 0;

        protected:
            /** @brief The properties of the window. */
            const WindowProps &_windowProps;

            /** @brief The event callback function for handling events. */
            EventCallbackFn _eventCallbackFn;
    };
} // namespace Vulkyrie
