#pragma once

#include "events/enums/key_code.h"
#include "events/enums/mouse_button.h"

namespace Vulkyrie {

    /** @brief Checks if a specific key is currently pressed.
     * @param key The key code to check.
     * @returns True if the key is pressed, false otherwise.
     */
    [[nodiscard]] bool IsKeyPressed(const KeyCode key);

    /** @brief Checks if a specific mouse button is currently pressed.
     * @param button The mouse button to check.
     * @returns True if the mouse button is pressed, false otherwise.
     */
    [[nodiscard]] bool IsMouseButtonPressed(const MouseButton button);

    /** @brief Gets the current position of the mouse cursor.
     * @returns A pair of floats representing the X and Y coordinates of the mouse cursor.
     */
    [[nodiscard]] std::pair<f32, f32> GetMousePosition();

} // namespace Vulkyrie
