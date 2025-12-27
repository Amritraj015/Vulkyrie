#pragma once

#include <vulkyrie.h>

namespace Pong {
    class PongGameLayer final : public Vulkyrie::Core::Layer {
        public:
            PongGameLayer(std::string_view layerName) : Vulkyrie::Core::Layer(layerName) {}
            ~PongGameLayer() = default;

            void OnAttach() override {
                VDEBUG("Layer Attached: %s", _layerName.c_str());
            }

            void OnDetach() override {
                VDEBUG("Layer Detached: %s",  _layerName.c_str());
            }

            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override {}
            void OnRender() override {}

            void OnEvent(Vulkyrie::Events::Event &event) override {
                VINFO("{} - Event",  _layerName.c_str());

                Vulkyrie::Events::EventDispatcher dispatcher(event);
                dispatcher.Dispatch<Vulkyrie::Events::KeyPressedEvent>([this](Vulkyrie::Events::KeyPressedEvent &e) {
                    if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::J) {
                        VINFO("J key pressed in game layer!");
                        // VINFO("%s - Event: %s",  _layerName.c_str(), event.ToString().c_str());

                        return true;
                    }

                    return false;
                });
            }
    };
}