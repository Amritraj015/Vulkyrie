#pragma once

#include <vulkyrie.h>
#include "pong_layer_0.h"
#include "pong_layer_1.h"
#include "pong_layer_2.h"
#include "pong_layer_3.h"
#include "pong_layer_4.h"

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

                    if (e.KeyCode == KeyCode::K) {
                        showWireFrame = !showWireFrame;

                        if (showWireFrame) {
                            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        } else {
                            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        }
                    }

                    if (e.KeyCode == KeyCode::J) {
                        if (currentLayer == 0) {
                            _application.PushLayer<PongLayer0>(windowWidth, windowHeight);
                        } else if (currentLayer == 1) {
                            _application.PushLayer<PongLayer1>(windowWidth, windowHeight);
                        } else if (currentLayer == 2) {
                            _application.PushLayer<PongLayer2>(windowWidth, windowHeight);
                        } else if (currentLayer == 3) {
                            _application.PushLayer<PongLayer3>(windowWidth, windowHeight);
                        } else if (currentLayer == 4) {
                            _application.PushLayer<PongLayer4>(windowWidth, windowHeight);
                        }

                        currentLayer = (currentLayer + 1) % 5;

                        return true;
                    }

                    return false;
                });
            };

        private:
            bool showWireFrame = false;
            f32 windowHeight, windowWidth;
            u8 currentLayer = 1;
    };

} // namespace Pong
