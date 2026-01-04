#pragma once

#include <vulkyrie.h>

namespace Pong {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;

    class PongLayer1 final : public Vulkyrie::Core::Layer {
        public:
            PongLayer1(Vulkyrie::Core::Application &application, f32 windowWidth, f32 windowHeight);
            ~PongLayer1() = default;

            void OnAttach() override;
            void OnDetach() override;
            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override;
            void OnEvent(Vulkyrie::Events::Event &event) override;

        private:
            Ref<Texture2D> texture1;
            Ref<Texture2D> texture2;
            Ref<VertexBuffer> vertexBuffer;
            Ref<VertexArray> vertexArray;
            Ref<GraphicsShader> graphicsShader;
            Camera camera;
            f64 lastMouseX = 400.0f;
            f64 lastMouseY = 300.0f;
            bool firstMouseMove = true;
            f32 windowHeight, windowWidth;

            bool moveForward = false;
            bool moveBackward = false;
            bool moveLeft = false;
            bool moveRight = false;
    };
} // namespace Pong
