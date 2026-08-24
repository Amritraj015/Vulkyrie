#include <GLFW/glfw3.h>
#include "events/enums/key_code.h"
#include "events/enums/mouse_button.h"
#include "core/application.h"

namespace Vulkyrie {

    [[nodiscard]] bool IsKeyPressed(const KeyCode key) {
        return glfwGetKey(static_cast<GLFWwindow *>(Application::GetSingleton().GetWindowHandle()), static_cast<u16>(key)) == GLFW_PRESS;
    }

    [[nodiscard]] bool IsMouseButtonPressed(const MouseButton button) {
        return glfwGetMouseButton(static_cast<GLFWwindow *>(Application::GetSingleton().GetWindowHandle()), static_cast<u8>(button)) == GLFW_PRESS;
    }

    [[nodiscard]] std::pair<f32, f32> GetMousePosition() {
        double xpos, ypos;

        glfwGetCursorPos(static_cast<GLFWwindow *>(Application::GetSingleton().GetWindowHandle()), &xpos, &ypos);

        return { static_cast<f32>(xpos), static_cast<f32>(ypos) };
    }

} // namespace Vulkyrie
