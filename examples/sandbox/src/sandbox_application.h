#pragma once

#include <vulkyrie.h>
#include "sandbox_void_layer.h"

namespace Sandbox {

    class SandboxApplication : public Application {
        public:
            SandboxApplication(const WindowProps &windowProps)
                : Application(windowProps) {
            }

            ~SandboxApplication() override {
                VTRACE("Destroying application sandbox");
            }

            bool OnInit(WindowCreatedEvent &event) override {
                // PushLayer<SandboxLayerShadowMapping>();
                // PushLayer<SandboxLayerFrameBuffer>();
                // PushLayer<SandboxLayerCubes>();
                PushLayer<SandboxLayerSphere>();
                PushOverlay<SandboxVoidLayer>();

                return true;
            }
    };

} // namespace Sandbox
