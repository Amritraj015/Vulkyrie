#pragma once

#include <vulkyrie.h>

namespace Pong {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;

    class PongGameLayer final : public Vulkyrie::Core::Layer {
        public:
            PongGameLayer(const Vulkyrie::Core::Application &application, f32 windowWidth, f32 windowHeight);
            ~PongGameLayer() = default;

            void OnAttach() override;
            void OnDetach() override;
            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override;
            void OnEvent(Vulkyrie::Events::Event &event) override;

        private:
            Ref<VertexArray> objectVertexArray;
            Ref<VertexBuffer> objectVertexBuffer;
            GraphicsShader objectShader;

            Ref<VertexArray> lightVertexArray;
            Ref<VertexBuffer> lightVertexBuffer;
            GraphicsShader lightShader;

            Camera camera;
            f64 lastMouseX = 400.0f;
            f64 lastMouseY = 300.0f;
            bool firstMouseMove = true;
            f32 windowHeight, windowWidth;
            f32 dt = 0.0f; // Time between current frame and last frame
    };
} // namespace Pong
