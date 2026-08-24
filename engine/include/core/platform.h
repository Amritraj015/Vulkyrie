#pragma once

#include "vlkypch.h"
#include "core/types/application_types.h"
#include "core/status_codes.h"
#include "events/event.h"

namespace Vulkyrie {
    using EventCallbackFn = std::function<void(Event &)>;

    class Platform final {
    public:
        VE_DELETE_MOVE_AND_COPY(Platform);

        /** @brief Constructs a new Window with the given properties.
         * @param windowProps The properties for the window.
         * @param eventCallbackFn The callback function for handling events.
         */
        Platform(const WindowProps &windowProps, const EventCallbackFn &eventCallbackFn) noexcept;

        /** @brief Default constructor for the Window class. */
        ~Platform();

        /** @brief Gets the native window handle.
         * @returns A pointer to the native window.
         */
        [[nodiscard]] VE_INLINE void *GetWindowHandle() const {
            return pWindow;
        }

        /** @brief Creates a new window for the application.
         * @returns StatusCode indicating success or failure.
         * */
        [[nodiscard]] StatusCode CreateWindow();

        /** @brief Enables or disables vertical synchronization (VSync) for the window.
         * @param enable True to enable VSync, false to disable.
         */
        void SetVSync(bool enable);

        /** @brief A callback that is called on each frame update cycle. */
        void OnUpdate() const;

        /** @brief Gets the current time in seconds since the window was created.
         * @returns The current time in seconds.
         */
        [[nodiscard]] f32 GetTime() const;

        /** @brief Captures or releases the mouse cursor when the window gains or loses focus.
         * @param enable True to capture the mouse on focus, false to release it.
         */
        void CaptureMouseOnFocus(bool enable);

    protected:
        /** Handle for the window. */
        void *pWindow;

        /** @brief The properties of the window. */
        WindowProps mWindowProps;

        /** @brief The event callback function for handling events. */
        EventCallbackFn mEventCallbackFn;
    };

} // namespace Vulkyrie
