#pragma once

#include <vulkyrie.h>
#include "sandbox_layer_back_pack.h"
#include "sandbox_layer_cubes.h"
#include "sandbox_layer_phong_lighting.h"
#include "sandbox_layer_specular_map.h"
#include "sandbox_layer_terrain_generation.h"
#include "sandbox_layer_attenuation.h"
#include "sandbox_layer_planet.h"
#include "sandbox_skybox_layer.h"
#include "sandbox_depth_and_stencil_testing.h"

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;

    class SandboxVoidLayer final : public Vulkyrie::Core::Layer {
        public:
            SandboxVoidLayer(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application), windowWidth(windowWidth), windowHeight(windowHeight) {};

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
                            _application.SuspendLayer<SandboxLayerSkybox>();

                            if (_application.HasLayer<SandboxDepthAndStencilTesting>()) {
                                _application.ResumeLayer<SandboxDepthAndStencilTesting>();
                            } else {
                                _application.PushLayer<SandboxDepthAndStencilTesting>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 1) {
                            _application.SuspendLayer<SandboxDepthAndStencilTesting>();

                            if (_application.HasLayer<SandboxLayerAttenuation>()) {
                                _application.ResumeLayer<SandboxLayerAttenuation>();
                            } else {
                                _application.PushLayer<SandboxLayerAttenuation>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 2) {
                            _application.SuspendLayer<SandboxLayerAttenuation>();

                            if (_application.HasLayer<SandboxLayerPlanet>()) {
                                _application.ResumeLayer<SandboxLayerPlanet>();
                            } else {
                                _application.PushLayer<SandboxLayerPlanet>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 3) {
                            _application.SuspendLayer<SandboxLayerPlanet>();

                            if (_application.HasLayer<SandboxLayerCubes>()) {
                                _application.ResumeLayer<SandboxLayerCubes>();
                            } else {
                                _application.PushLayer<SandboxLayerCubes>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 4) {
                            _application.SuspendLayer<SandboxLayerCubes>();

                            if (_application.HasLayer<SandboxLayerPhongLighting>()) {
                                _application.ResumeLayer<SandboxLayerPhongLighting>();
                            } else {
                                _application.PushLayer<SandboxLayerPhongLighting>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 5) {
                            _application.SuspendLayer<SandboxLayerPhongLighting>();

                            if (_application.HasLayer<SandboxLayerSpecularMap>()) {
                                _application.ResumeLayer<SandboxLayerSpecularMap>();
                            } else {
                                _application.PushLayer<SandboxLayerSpecularMap>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 6) {
                            _application.SuspendLayer<SandboxLayerSpecularMap>();

                            if (_application.HasLayer<SandboxLayerTerrainGeneration>()) {
                                _application.ResumeLayer<SandboxLayerTerrainGeneration>();
                            } else {
                                _application.PushLayer<SandboxLayerTerrainGeneration>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 7) {
                            _application.SuspendLayer<SandboxLayerTerrainGeneration>();

                            if (_application.HasLayer<SandboxLayerBackPack>()) {
                                _application.ResumeLayer<SandboxLayerBackPack>();
                            } else {
                                _application.PushLayer<SandboxLayerBackPack>(windowWidth, windowHeight);
                            }
                        } else if (currentLayer == 8) {
                            _application.SuspendLayer<SandboxLayerBackPack>();

                            if (_application.HasLayer<SandboxLayerSkybox>()) {
                                _application.ResumeLayer<SandboxLayerSkybox>();
                            } else {
                                _application.PushLayer<SandboxLayerSkybox>(windowWidth, windowHeight);
                            }
                        }

                        currentLayer = (currentLayer + 1) % 9;

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
} // namespace Sandbox
