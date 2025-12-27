#pragma once

#include "core/window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie::Core {
    class GenericWindow final : public Window {
        public:
            GenericWindow(const Vulkyrie::Core::Application &application);
            ~GenericWindow() = default;

            Vulkyrie::Core::StatusCode Create() override;

            Vulkyrie::Core::StatusCode Close() override;

            void ToggleWireframeMode(bool enable) override;

        private:
            GLFWwindow *_window;
    };
} // namespace Vulkyrie::Core
