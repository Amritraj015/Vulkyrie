#pragma once

#include <vulkyrie.h>
#include "sandbox_void_layer.h"

namespace Sandbox {
    class SandboxApplication : public Vulkyrie::Core::Application {
        public:
            SandboxApplication(const Vulkyrie::Core::WindowProps &windowProps) : Vulkyrie::Core::Application(windowProps) {
            }

            ~SandboxApplication() override = default;

            bool OnInit(Vulkyrie::Events::WindowCreatedEvent &event) override {
                PushLayer<SandboxLayerTerrainGeneration>(event.Width, event.Height);
                PushOverlay<SandboxVoidLayer>(event.Width, event.Height);

                return true;
            }
    };
} // namespace Sandbox
