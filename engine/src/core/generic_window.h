#pragma once

#include "core/window.h"
#include "core/window_props.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie::Core {
    class GenericWindow final : public Window {
        public:
            GenericWindow(const Vulkyrie::Core::WindowProps &windowProps, const EventCallbackFn &eventCallbackFn);
            ~GenericWindow() override;

            Vulkyrie::Core::StatusCode Create() override;

            void SetVSync(bool enabled) override;

            inline void OnUpdate() const override;

            Vulkyrie::Core::StatusCode Close() override;

            inline void ToggleWireframeMode(bool enable) override;

            inline void CaptureMouseOnFocus(bool enable) override;

            [[nodiscard]] inline float GetTime() const override {
                return static_cast<float>(glfwGetTime());
            }

        private:
            GLFWwindow *_window;
    };
} // namespace Vulkyrie::Core
