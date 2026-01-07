#pragma once

#include "core/window_props.h"
#include "core/status_codes.h"
#include "events/event.h"
#include "events/enums/key_code.h"
#include "events/enums/mouse_button.h"

namespace Vulkyrie::Core {
    using EventCallbackFn = std::function<void(Vulkyrie::Events::Event &)>;

    class Platform {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Platform(const Platform &) = delete;
            void operator=(Platform const &) = delete;

            /** @brief Constructs a new Window with the given properties.
             * @param windowProps The properties for the window.
             * @param eventCallbackFn The callback function for handling events.
             */
            Platform(const Vulkyrie::Core::WindowProps &windowProps, const EventCallbackFn &eventCallbackFn) noexcept
                : _windowProps(windowProps), _eventCallbackFn(eventCallbackFn) {
            }

            /** @brief Default constructor for the Window class. */
            virtual ~Platform() = default;

            /** @brief Creates a new window for the application.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure.
             * */
            virtual Vulkyrie::Core::StatusCode CreateWindow() = 0;

            /** @brief Enables or disables vertical synchronization (VSync) for the window.
             * @param enabled True to enable VSync, false to disable.
             */
            virtual inline void SetVSync(bool enabled) = 0;

            /** @brief A callback that is called on each frame update cycle. */
            virtual inline void OnUpdate() const = 0;

            /** @brief Closes the application window.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure.
             * */
            virtual Vulkyrie::Core::StatusCode Close() = 0;

            /** @brief Gets the current time in seconds since the window was created.
             * @returns The current time in seconds.
             */
            [[nodiscard]] virtual inline f32 GetTime() const = 0;

            /** @brief Toggles wireframe rendering mode.
             * @param enable True to enable wireframe mode, false to disable.
             */
            virtual inline void ToggleWireframeMode(bool enable) = 0;

            /** @brief Captures or releases the mouse cursor when the window gains or loses focus.
             * @param enable True to capture the mouse on focus, false to release it.
             */
            virtual inline void CaptureMouseOnFocus(bool enable) = 0;

            /** @brief Checks if a specific key is currently pressed.
             * @param key The key code to check.
             * @returns True if the key is pressed, false otherwise.
             */
            [[nodiscard]] virtual inline bool IsKeyPressed(const Vulkyrie::Events::KeyCode key) const = 0;

            /** @brief Checks if a specific mouse button is currently pressed.
             * @param button The mouse button to check.
             * @returns True if the mouse button is pressed, false otherwise.
             */
            [[nodiscard]] virtual inline bool IsMouseButtonPressed(const Vulkyrie::Events::MouseButton button) const = 0;

            /** @brief Gets the current position of the mouse cursor.
             * @returns A pair of floats representing the X and Y coordinates of the mouse cursor.
             */
            [[nodiscard]] virtual inline std::pair<f32, f32> GetMousePosition() const = 0;

            /** @brief Gets the current X position of the mouse cursor.
             * @returns The X coordinate of the mouse cursor.
             */
            [[nodiscard]] virtual inline f32 GetMouseX() const = 0;

            /** @brief Gets the current Y position of the mouse cursor.
             * @returns The Y coordinate of the mouse cursor.
             */
            [[nodiscard]] virtual inline f32 GetMouseY() const = 0;

        protected:
            const Vulkyrie::Core::WindowProps &_windowProps;
            EventCallbackFn _eventCallbackFn;
    };
} // namespace Vulkyrie::Core
