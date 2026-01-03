#pragma once

#include <vulkyrie.h>
#include "pong_layer_1.h"
#include "pong_layer_2.h"
#include "pong_layer_3.h"

namespace Pong {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;

    class PongVoidLayer final : public Vulkyrie::Core::Layer {
        public:
            PongVoidLayer(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application), windowWidth(windowWidth), windowHeight(windowHeight) {};
            ~PongVoidLayer() = default;

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
                    constexpr float cameraSpeed = 30.0f; // adjust accordingly

                    if (e.KeyCode == KeyCode::J || e.KeyCode == KeyCode::K || e.KeyCode == KeyCode::L) {
                        _application.PopLayer<PongLayer1>();
                        _application.PopLayer<PongLayer2>();
                        _application.PopLayer<PongLayer3>();

                        if (e.KeyCode == KeyCode::J) {
                            _application.PushLayer<PongLayer1>(windowWidth, windowHeight);
                        } else if (e.KeyCode == KeyCode::K) {
                            _application.PushLayer<PongLayer2>(windowWidth, windowHeight);
                        } else if (e.KeyCode == KeyCode::L) {
                            _application.PushLayer<PongLayer3>(windowWidth, windowHeight);
                        }

                        return true;
                    }

                    return false;
                });
            };

        private:
            f32 windowHeight, windowWidth;
    };

} // namespace Pong
