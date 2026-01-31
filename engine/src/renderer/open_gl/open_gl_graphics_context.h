#pragma once

#include "renderer/graphics_context.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie::Renderer {
    class OpenGLGraphicsContext final : public GraphicsContext {
        public:
            /** @brief Constructs a new OpenGLGraphicsContext with the given window handle.
             * @param windowHandle The handle to the window.
             */
            OpenGLGraphicsContext(void *windowHandle);

            Vulkyrie::Core::StatusCode Initialize() override;
            void SwapBuffers() override;

        private:
            GLFWwindow *_windowHandle;
    };
} // namespace Vulkyrie::Core
