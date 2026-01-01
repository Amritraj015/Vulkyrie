#pragma once

#include "core/window.h"
#include "core/window_props.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie::Core {
    class GenericWindow final : public Window {
        public:
            GenericWindow(const Vulkyrie::Core::WindowProps &windowProps, const EventCallbackFn &eventCallbackFn);
            ~GenericWindow() override = default;

            Vulkyrie::Core::StatusCode Create() override;
            void SetVSync(bool enabled) override;
            inline void OnUpdate() const override;
            Vulkyrie::Core::StatusCode Close() override;
            void ToggleWireframeMode(bool enable) override;

        private:
            GLFWwindow *_window;
    };
} // namespace Vulkyrie::Core
