#pragma once

#include <vulkyrie.h>
#include "sandbox_layer_back_pack.h"
#include "sandbox_layer_cubes.h"
#include "sandbox_layer_frame_buffer.h"
#include "sandbox_layer_phong_lighting.h"
#include "sandbox_layer_specular_map.h"
#include "sandbox_layer_terrain_generation.h"
#include "sandbox_layer_attenuation.h"
#include "sandbox_layer_planet.h"
#include "sandbox_layer_skybox.h"
#include "sandbox_layer_depth_and_stencil_testing.h"

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;

    class SandboxVoidLayer final : public Vulkyrie::Core::Layer {
        public:
            SandboxVoidLayer() {}
            ~SandboxVoidLayer() = default;

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
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
                            Application::GetSingleton().SuspendLayer<SandboxLayerSkybox>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerFrameBuffer>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerFrameBuffer>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerFrameBuffer>();
                            }
                        } else if (currentLayer == 1) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerFrameBuffer>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerDepthAndStencilTesting>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerDepthAndStencilTesting>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerDepthAndStencilTesting>();
                            }
                        } else if (currentLayer == 2) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerDepthAndStencilTesting>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerAttenuation>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerAttenuation>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerAttenuation>();
                            }
                        } else if (currentLayer == 3) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerAttenuation>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerPlanet>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerPlanet>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerPlanet>();
                            }
                        } else if (currentLayer == 4) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerPlanet>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerCubes>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerCubes>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerCubes>();
                            }
                        } else if (currentLayer == 5) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerCubes>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerPhongLighting>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerPhongLighting>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerPhongLighting>();
                            }
                        } else if (currentLayer == 6) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerPhongLighting>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerSpecularMap>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerSpecularMap>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerSpecularMap>();
                            }
                        } else if (currentLayer == 7) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerSpecularMap>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerTerrainGeneration>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerTerrainGeneration>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerTerrainGeneration>();
                            }
                        } else if (currentLayer == 8) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerTerrainGeneration>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerBackPack>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerBackPack>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerBackPack>();
                            }
                        } else if (currentLayer == 9) {
                            Application::GetSingleton().SuspendLayer<SandboxLayerBackPack>();

                            if (Application::GetSingleton().HasLayer<SandboxLayerSkybox>()) {
                                Application::GetSingleton().ResumeLayer<SandboxLayerSkybox>();
                            } else {
                                Application::GetSingleton().PushLayer<SandboxLayerSkybox>();
                            }
                        }

                        currentLayer = (currentLayer + 1) % 10;

                        return true;
                    }

                    return false;
                });
            };

        private:
            bool showWireFrame = false;
            u8 currentLayer = 1;
    };
} // namespace Sandbox
