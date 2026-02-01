#pragma once
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {

    class OpenGLRenderer : public Renderer {
        public:
            OpenGLRenderer() = default;
            ~OpenGLRenderer() override = default;

            void BeginScene([[maybe_unused]] const Camera &camera) override {
            }
            void EndScene() override {
            }

            Scope<CommandBuffer> CreateCommandBuffer() override {
                // Implementation for creating a command buffer in OpenGL.
                return nullptr; // Placeholder
            }

        private:
            Scope<CommandBuffer> _commandBuffer;
    };

} // namespace Vulkyrie::Renderer
