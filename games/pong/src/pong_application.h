#pragma once

#include <vulkyrie.h>
#include "pong_void_layer.h"

namespace Pong {
    class PongApplication : public Vulkyrie::Core::Application {
        public:
            PongApplication(const Vulkyrie::Core::WindowProps &windowProps) : Vulkyrie::Core::Application(windowProps) {
            }

            ~PongApplication() override = default;

            bool OnInit(Vulkyrie::Events::WindowCreatedEvent &event) override {
                PushLayer<Pong::PongLayer4>(event.Width, event.Height);
                PushOverlay<Pong::PongVoidLayer>(event.Width, event.Height);

                return true;
            }
    };
} // namespace Pong
