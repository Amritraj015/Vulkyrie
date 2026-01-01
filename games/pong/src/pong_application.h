#pragma once

#include <vulkyrie.h>
#include "pong_overlay_layer.h"
#include "pong_game_layer.h"

namespace Pong {
    class PongApplication : public Vulkyrie::Core::Application {
        public:
            PongApplication(Vulkyrie::Core::WindowProps windowProps, Vulkyrie::Core::ApplicationConfig config)
                : Vulkyrie::Core::Application(windowProps, config) {
            }

            ~PongApplication();

            bool OnInit(Vulkyrie::Events::WindowCreatedEvent &event) override {
                PushLayer<Pong::PongGameLayer>("Pong Game Layer");
                PushOverlay<Pong::PongOverlayLayer>("Pong Overlay Layer");

                VINFO("PongApplication Initialized!");

                return true;
            }
    };
} // namespace Pong
