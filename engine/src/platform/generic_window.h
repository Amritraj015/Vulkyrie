#pragma once

#include "window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie::Platform {
    class GenericWindow final : public Window {
        public:
            GenericWindow(const Vulkyrie::Core::Application &application);
            ~GenericWindow() = default;

            /** Creates a new window for the application.  */
            Vulkyrie::Core::StatusCode Create() override;

            /** Closes the application window. */
            Vulkyrie::Core::StatusCode Close() override;

        private:
            GLFWwindow *_window;
    };
} // namespace Vulkyrie::Platform
