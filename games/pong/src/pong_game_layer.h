#pragma once

#include <vulkyrie.h>

namespace Pong {
    class PongGameLayer final : public Vulkyrie::Core::Layer {
        public:
            PongGameLayer(std::string_view layerName) : Vulkyrie::Core::Layer(layerName) {}
            ~PongGameLayer() = default;

            void OnAttach() override {
                VDEBUG("Layer Attached: {}", _layerName.c_str());
            }

            void OnDetach() override {
                VDEBUG("Layer Detached: {}",  _layerName.c_str());
            }

            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override {}
            void OnRender() override {}

            void OnEvent(Vulkyrie::Events::Event &event) override {
                VINFO("{} - Event",  _layerName.c_str());

                Vulkyrie::Events::EventDispatcher dispatcher(event);
                dispatcher.Dispatch<Vulkyrie::Events::KeyPressedEvent>([this](Vulkyrie::Events::KeyPressedEvent &e) {
                    if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::J) {
                        VINFO("J key pressed in game layer!");

                        return true;
                    }

                    return false;
                });
            }
    };
}