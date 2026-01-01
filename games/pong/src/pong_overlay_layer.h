#pragma once

#include <vulkyrie.h>

namespace Pong {
    class PongOverlayLayer final : public Vulkyrie::Core::Layer {
        public:
            PongOverlayLayer(const std::string_view layerName) : Vulkyrie::Core::Layer(layerName) {
            }
            ~PongOverlayLayer() = default;

            void OnAttach() override {
                VDEBUG("Layer Attached: {}", _layerName.data());
            }

            void OnDetach() override {
                VDEBUG("Layer Detached: {}", _layerName.data());
            }

            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override {
            }

            // void OnRender() override {
            // }

            void OnEvent(Vulkyrie::Events::Event &event) override {
                // VINFO("{} - Event: {}",  _layerName.c_str(), event.ToString());
                Vulkyrie::Events::EventDispatcher dispatcher(event);

                dispatcher.Dispatch<Vulkyrie::Events::KeyPressedEvent>([this](Vulkyrie::Events::KeyPressedEvent &e) {
                    if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::J) {
                        _toggleWireframe = !_toggleWireframe;

                        VINFO("J key pressed in {}!", _layerName.data());
                        // Vulkyrie::Core::ApplicationManager::ToggleWireframeMode(_toggleWireframe);

                        return true;
                    }

                    return false;
                });
            }

        private:
            bool _toggleWireframe = false;
    };
} // namespace Pong
