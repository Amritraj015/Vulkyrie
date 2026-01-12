#pragma once

#include <vulkyrie.h>

namespace Pong {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;

    class PongLayer2 final : public Vulkyrie::Core::Layer {
        public:
            PongLayer2(Vulkyrie::Core::Application &application, f32 windowWidth, f32 windowHeight);
            ~PongLayer2() = default;

            void OnAttach() override;
            void OnDetach() override;
            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override;
            void OnEvent(Vulkyrie::Events::Event &event) override;

        private:
            Ref<VertexArray> objectVertexArray;
            Ref<VertexBuffer> objectVertexBuffer;
            Ref<Shader> objectShader;

            Ref<VertexArray> lightVertexArray;
            Ref<VertexBuffer> lightVertexBuffer;
            Ref<Shader> lightShader;

            Camera camera;
            f64 lastMouseX = 400.0f;
            f64 lastMouseY = 300.0f;
            bool firstMouseMove = true;
            f32 windowHeight, windowWidth;
    };
} // namespace Pong
