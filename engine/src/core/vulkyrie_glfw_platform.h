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

            Vulkyrie::Core::StatusCode CreateWindow() override;
            Vulkyrie::Core::StatusCode Close() override;

            inline void SetVSync(bool enabled) override {
                glfwSwapInterval(_windowProps.VSync ? 1 : 0);
            }

            inline void OnUpdate() const override {
                glfwSwapBuffers(_window);
                glfwPollEvents();
            }

            inline void ToggleWireframeMode(bool enable) override {
                if (enable) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                } else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
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

            [[nodiscard]] inline bool IsKeyPressed(const Vulkyrie::Events::KeyCode key) const override {
                return glfwGetKey(_window, static_cast<u16>(key)) == GLFW_PRESS;
            }

            [[nodiscard]] inline bool IsMouseButtonPressed(const Vulkyrie::Events::MouseButton button) const override {
                return glfwGetMouseButton(_window, static_cast<u8>(button)) == GLFW_PRESS;
            }

            [[nodiscard]] inline std::pair<f32, f32> GetMousePosition() const override {
                double xpos, ypos;
                glfwGetCursorPos(_window, &xpos, &ypos);

                return { (f32)xpos, (f32)ypos };
            }

            [[nodiscard]] inline f32 GetMouseX() const override {
                return GetMousePosition().first;
            }

            [[nodiscard]] inline f32 GetMouseY() const override {
                return GetMousePosition().second;
            }

        private:
            GLFWwindow *_window;
            Scope<GraphicsContext> _graphicsContext;
    };
} // namespace Vulkyrie::Core
