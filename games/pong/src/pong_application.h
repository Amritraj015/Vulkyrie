#pragma once

#include <vulkyrie.h>
#include "pong_overlay_layer.h"
#include "pong_game_layer.h"

namespace Pong {
    class PongApplication : public Vulkyrie::Core::Application {
        public:
            PongApplication(const Vulkyrie::Core::WindowProps &windowProps, const Vulkyrie::Core::ApplicationConfig config)
                : Vulkyrie::Core::Application(windowProps, config) {
            }

            ~PongApplication() override = default;

            bool OnInit(Vulkyrie::Events::WindowCreatedEvent &event) override {
                PushLayer<Pong::PongGameLayer>(event.Width, event.Height);
                // PushLayer<Pong::PongOverlayLayer>(event.Width, event.Height);

                VINFO("PongApplication Initialized!");

                return true;
            }
    };
} // namespace Pong
