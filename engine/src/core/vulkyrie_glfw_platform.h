#pragma once

#include "core/platform.h"
#include "core/window_props.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie {
    class VulkyrieGLFWPlatform final : public Platform {
    public:
        VulkyrieGLFWPlatform(const WindowProps &windowProps, const EventCallbackFn &eventCallbackFn);
        ~VulkyrieGLFWPlatform() override;

        [[nodiscard]] StatusCode CreateWindow() override;
        StatusCode CloseWindow() override;

        void SetVSync(bool enable) override;

        VE_INLINE void OnUpdate() const override {
            glfwSwapBuffers(_window);
            glfwPollEvents();
        }

        void CaptureMouseOnFocus(bool enable) override;

        [[nodiscard]] VE_INLINE f32 GetTime() const override {
            return static_cast<f32>(glfwGetTime());
        }

        [[nodiscard]] VE_INLINE void *GetWindowHandle() const override {
            return _window;
        }

    private:
        GLFWwindow *_window;
    };
} // namespace Vulkyrie
