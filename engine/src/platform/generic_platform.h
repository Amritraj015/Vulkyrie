#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "platform_base.h"

namespace Vulkyrie::Platform {
    class GenericPlatform final : public PlatformBase {
        public:
            GenericPlatform() = default;
            ~GenericPlatform() override = default;

            Vulkyrie::Core::StatusCode CreateNewWindow(Vulkyrie::Core::VulkyrieWindowProps props) override;
            Vulkyrie::Core::StatusCode CloseWindow() override;
            // bool PollForEvents() override;
            // void SleepForDuration(u64 duration) override;
        
        private:
            GLFWwindow *window;
            GLuint VBO, VAO, shaderProgram;
    };
} // namespace Vulkyrie::Platform
