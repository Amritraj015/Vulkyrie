#pragma once

#include <vulkyrie.h>
#include "sandbox_layer_back_pack.h"
#include "sandbox_layer_cubes.h"
#include "sandbox_layer_phong_lighting.h"
#include "sandbox_layer_specular_map.h"
#include "sandbox_layer_terrain_generation.h"
#include "sandbox_layer_attenuation.h"
#include "sandbox_layer_planet.h"

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
                            _application.PopLayer<SandboxLayerAttenuation>();
                            _application.PushLayer<SandboxLayerBackPack>(windowWidth, windowHeight);
                        } else if (currentLayer == 1) {
                            _application.PopLayer<SandboxLayerBackPack>();
                            _application.PushLayer<SandboxLayerPlanet>(windowWidth, windowHeight);
                        } else if (currentLayer == 2) {
                            _application.PopLayer<SandboxLayerPlanet>();
                            _application.PushLayer<SandboxLayerCubes>(windowWidth, windowHeight);
                        } else if (currentLayer == 3) {
                            _application.PopLayer<SandboxLayerCubes>();
                            _application.PushLayer<SandboxLayerPhongLighting>(windowWidth, windowHeight);
                        } else if (currentLayer == 4) {
                            _application.PopLayer<SandboxLayerPhongLighting>();
                            _application.PushLayer<SandboxLayerSpecularMap>(windowWidth, windowHeight);
                        } else if (currentLayer == 5) {
                            _application.PopLayer<SandboxLayerSpecularMap>();
                            _application.PushLayer<SandboxLayerTerrainGeneration>(windowWidth, windowHeight);
                        } else if (currentLayer == 6) {
                            _application.PopLayer<SandboxLayerTerrainGeneration>();
                            _application.PushLayer<SandboxLayerAttenuation>(windowWidth, windowHeight);
                        }

                        currentLayer = (currentLayer + 1) % 7;

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
