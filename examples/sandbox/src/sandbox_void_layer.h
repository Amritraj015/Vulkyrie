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
            SandboxVoidLayer() {
                InitializeLayerSwitcher();
            }
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
                        SwitchToNextLayer();
                        return true;
                    }

                    return false;
                });
            };

        private:
            bool showWireFrame = false;
            u8 currentLayer = 1;

            // Layer switcher function type
            using LayerSwitchFn = std::function<void()>;
            std::vector<LayerSwitchFn> layerSwitchers;

            void InitializeLayerSwitcher() {
                layerSwitchers = {
                    [this]() { SwitchLayer<SandboxLayerSkybox>(); },
                    [this]() { SwitchLayer<SandboxLayerFrameBuffer>(); },
                    [this]() { SwitchLayer<SandboxLayerDepthAndStencilTesting>(); },
                    [this]() { SwitchLayer<SandboxLayerAttenuation>(); },
                    [this]() { SwitchLayer<SandboxLayerPlanet>(); },
                    [this]() { SwitchLayer<SandboxLayerCubes>(); },
                    [this]() { SwitchLayer<SandboxLayerPhongLighting>(); },
                    [this]() { SwitchLayer<SandboxLayerSpecularMap>(); },
                    [this]() { SwitchLayer<SandboxLayerTerrainGeneration>(); },
                    [this]() { SwitchLayer<SandboxLayerBackPack>(); }
                };
            }

            template<typename T>
            void SwitchLayer() {
                auto& app = Application::GetSingleton();
                
                if (app.HasLayer<T>()) {
                    app.ResumeLayer<T>();
                } else {
                    app.PushLayer<T>();
                }
            }

            void SwitchToNextLayer() {
                auto& app = Application::GetSingleton();
                
                // Suspend all known layers
                app.SuspendLayer<SandboxLayerSkybox>();
                app.SuspendLayer<SandboxLayerFrameBuffer>();
                app.SuspendLayer<SandboxLayerDepthAndStencilTesting>();
                app.SuspendLayer<SandboxLayerAttenuation>();
                app.SuspendLayer<SandboxLayerPlanet>();
                app.SuspendLayer<SandboxLayerCubes>();
                app.SuspendLayer<SandboxLayerPhongLighting>();
                app.SuspendLayer<SandboxLayerSpecularMap>();
                app.SuspendLayer<SandboxLayerTerrainGeneration>();
                app.SuspendLayer<SandboxLayerBackPack>();
                
                // Move to next layer
                currentLayer = (currentLayer + 1) % layerSwitchers.size();
                
                // Activate the new layer
                layerSwitchers[currentLayer]();
            }
    };
} // namespace Sandbox
