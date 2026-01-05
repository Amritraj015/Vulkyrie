#pragma once

#include <vulkyrie.h>
#include "pong_void_layer.h"

namespace Pong {
    class PongApplication : public Vulkyrie::Core::Application {
        public:
            PongApplication(const Vulkyrie::Core::WindowProps &windowProps, const Vulkyrie::Core::ApplicationConfig config) : Vulkyrie::Core::Application(windowProps, config) {
            }

            ~PongApplication() override = default;

            bool OnInit(Vulkyrie::Events::WindowCreatedEvent &event) override {
                PushLayer<Pong::PongLayer0>(event.Width, event.Height);
                PushOverlay<Pong::PongVoidLayer>(event.Width, event.Height);

                return true;
            }
    };
} // namespace Pong
