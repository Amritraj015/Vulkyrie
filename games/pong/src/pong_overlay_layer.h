#pragma once

#include <vulkyrie.h>

namespace Pong {
    class PongOverlayLayer final : public Vulkyrie::Core::Layer {
        public:
            PongOverlayLayer(std::string_view layerName) : Vulkyrie::Core::Layer(layerName) {}
            ~PongOverlayLayer() = default;

            void OnAttach() override {
                VDEBUG("Layer Attached: %s", _layerName.c_str());
            }

            void OnDetach() override {
                VDEBUG("Layer Detached: %s",  _layerName.c_str());
            }

            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override {

            }

            void OnRender() override {

            }

            void OnEvent(Vulkyrie::Events::Event &event) override {

            }
    };
}