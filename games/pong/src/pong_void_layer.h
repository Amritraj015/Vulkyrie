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

                    if (e.KeyCode == KeyCode::J) {
                        _application.PopLayer<PongLayer1>();
                        _application.PopLayer<PongLayer2>();
                        _application.PopLayer<PongLayer3>();

                        if (currentLayer == 0) {
                            _application.PushLayer<PongLayer1>(windowWidth, windowHeight);
                        } else if (currentLayer == 1) {
                            _application.PushLayer<PongLayer2>(windowWidth, windowHeight);
                        } else if (currentLayer == 2) {
                            _application.PushLayer<PongLayer3>(windowWidth, windowHeight);
                        }

                        currentLayer = (currentLayer + 1) % 3;

                        return true;
                    }

                    return false;
                });
            };

        private:
            f32 windowHeight, windowWidth;
            u8 currentLayer = 1;
    };

} // namespace Pong
