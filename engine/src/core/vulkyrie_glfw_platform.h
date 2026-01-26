#pragma once

#include "core/platform.h"
#include "core/window_props.h"
#include "core/graphics_context.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie::Core {
    class VulkyrieGLFWPlatform final : public Platform {
        public:
            VulkyrieGLFWPlatform(const Vulkyrie::Core::WindowProps &windowProps, const EventCallbackFn &eventCallbackFn);
            ~VulkyrieGLFWPlatform() override;

            [[nodiscard]] StatusCode CreateWindow() override;
            StatusCode Close() override;

            inline void SetVSync(bool enabled) override {
                glfwSwapInterval(static_cast<i32>(enabled));
            }

            inline void OnUpdate() const override {
                glfwSwapBuffers(_window);
                glfwPollEvents();
            }

            inline void CaptureMouseOnFocus(bool enable) override {
                if (enable) {
                    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                } else {
                    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }

            [[nodiscard]] inline f32 GetTime() const override {
                return static_cast<float>(glfwGetTime());
            }

            [[nodiscard]] inline void *GetWindowHandle() const override {
                return _window;
            }

        private:
            GLFWwindow *_window;
            Scope<GraphicsContext> _graphicsContext;
    };
} // namespace Vulkyrie::Core
