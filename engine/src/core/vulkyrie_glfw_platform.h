#pragma once

#include "core/platform.h"
#include "core/window_props.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie::Core {
    class VulkyrieGLFWPlatform final : public Platform {
        public:
            VulkyrieGLFWPlatform(const Vulkyrie::Core::WindowProps &windowProps, const EventCallbackFn &eventCallbackFn);
            ~VulkyrieGLFWPlatform() override;

            Vulkyrie::Core::StatusCode CreateWindow() override;

            inline void SetVSync(bool enabled) override;

            inline void OnUpdate() const override;

            Vulkyrie::Core::StatusCode Close() override;

            inline void ToggleWireframeMode(bool enable) override;

            inline void CaptureMouseOnFocus(bool enable) override;

            [[nodiscard]] inline f32 GetTime() const override {
                return static_cast<float>(glfwGetTime());
            }

            [[nodiscard]] inline bool IsKeyPressed(const Vulkyrie::Events::KeyCode key) const override;
            [[nodiscard]] inline bool IsMouseButtonPressed(const Vulkyrie::Events::MouseButton button) const override;
            [[nodiscard]] inline std::pair<f32, f32> GetMousePosition() const override;
            [[nodiscard]] inline f32 GetMouseX() const override;
            [[nodiscard]] inline f32 GetMouseY() const override;

        private:
            GLFWwindow *_window;
    };
} // namespace Vulkyrie::Core
