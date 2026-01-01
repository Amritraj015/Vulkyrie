#include "GLFW/glfw3.h"
#include "platform.h"

namespace Vulkyrie::Platform {
    f32 GetTime() {
        return static_cast<float>(glfwGetTime());
    }
} // namespace Vulkyrie::Platform
