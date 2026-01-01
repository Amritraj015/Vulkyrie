#pragma once

#include <vulkyrie.h>

namespace Pong {
    class PongOverlayLayer final : public Vulkyrie::Core::Layer {
        public:
            PongOverlayLayer(const std::string_view layerName);
            ~PongOverlayLayer() = default;

            void OnAttach() override;
            void OnDetach() override;
            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override;
            void OnEvent(Vulkyrie::Events::Event &event) override;

        private:
            bool _toggleWireframe = false;
            Ref<Vulkyrie::Renderer::VertexArray> _vertexArray;
    };
} // namespace Pong
