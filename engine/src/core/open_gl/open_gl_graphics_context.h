#pragma once

#include "core/logger.h"
#include "core/graphics_context.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Vulkyrie::Core {
    class OpenGLGraphicsContext final : public GraphicsContext {
        public:
            /** @brief Constructs a new OpenGLGraphicsContext with the given window handle.
             * @param windowHandle The handle to the window.
             */
            OpenGLGraphicsContext(void *windowHandle) : _windowHandle(static_cast<GLFWwindow *>(windowHandle)) {
            }

            StatusCode Initialize() override {
                // Make the OpenGL context current.
                glfwMakeContextCurrent(_windowHandle);

                // GLAD: load all OpenGL function pointers
                if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
                    VFATAL("Failed to initialize GLAD");

                    return StatusCode::FailedToInitializeGLAD;
                }

                return StatusCode::Successful;
            };

            inline void SwapBuffers() override {
                glfwSwapBuffers(_windowHandle);
            }

        private:
            GLFWwindow *_windowHandle;
    };
} // namespace Vulkyrie::Core
