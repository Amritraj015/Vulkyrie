#pragma once

#include <vulkyrie.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "sandbox_layer_back_pack.h"
#include "sandbox_layer_cubes.h"
#include "sandbox_layer_blinn_phong_lighting.h"
#include "sandbox_layer_deferred_shading.h"
#include "sandbox_layer_frame_buffer.h"
#include "sandbox_layer_phong_lighting.h"
#include "sandbox_layer_normal_mapping.h"
#include "sandbox_layer_specular_map.h"
#include "sandbox_layer_terrain_generation.h"
#include "sandbox_layer_attenuation.h"
#include "sandbox_layer_planet.h"
#include "sandbox_layer_shadow_mapping.h"
#include "sandbox_layer_skybox.h"
#include "sandbox_layer_depth_and_stencil_testing.h"
#include "sandbox_layer_sphere.h"

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;

    class SandboxVoidLayer final : public Layer {
        public:
            SandboxVoidLayer()
                : app(Application::GetSingleton()) {
                InitializeLayerSwitcher();
            }

            ~SandboxVoidLayer() = default;

            void OnAttached() override {
                // Setup Dear ImGui context
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO &io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
                io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch

                // Setup Platform/Renderer backends
                // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
                ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow *>(app.GetWindowHandle()), true);
                ImGui_ImplOpenGL3_Init("#version 460");
            }

            void OnDetached() override {
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
            }

            void OnUpdate(Timestep deltaTime) override {
                // Start the Dear ImGui frame
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                {
                    ImGui::ShowDemoWindow(); // Show demo window! :)
                }

                // Rendering
                // (Your code clears your framebuffer, renders your other stuff etc.)
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) { return !captureMouseOnFocus; });

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
                    if (e.KeyCode == KeyCode::Escape) {
                        app.Stop();
                        return true;
                    }

                    if (e.KeyCode == KeyCode::E) {
                        enableVSync = !enableVSync;
                        glfwSwapInterval(static_cast<i32>(enableVSync));
                    }

                    if (e.KeyCode == KeyCode::Q) {
                        captureMouseOnFocus = !captureMouseOnFocus;
                        app.CaptureMouseOnFocus(captureMouseOnFocus);
                    }

                    if (e.KeyCode == KeyCode::K) {
                        showWireFrame = !showWireFrame;
                        glPolygonMode(GL_FRONT_AND_BACK, showWireFrame ? GL_LINE : GL_FILL);
                    }

                    if (e.KeyCode == KeyCode::J) {
                        SwitchToNextLayer(true);
                        return true;
                    }

                    if (e.KeyCode == KeyCode::H) {
                        SwitchToNextLayer(false);
                        return true;
                    }

                    return false;
                });
            };

        private:
            Application &app;
            bool captureMouseOnFocus = true;
            bool enableVSync = false;
            bool showWireFrame = false;
            u8 currentLayer = 1;

            // Layer switcher function type
            using LayerSwitchFn = std::function<void()>;
            std::vector<LayerSwitchFn> layerSwitchers;

            void InitializeLayerSwitcher() {
                layerSwitchers = {
                    [this]() { SwitchLayer<SandboxLayerFrameBuffer>(); },            // Frame buffer example.
                    [this]() { SwitchLayer<SandboxLayerSphere>(); },                 // Sphere example.
                    [this]() { SwitchLayer<SandboxLayerDeferredShading>(); },        // Deferred shading example.
                    [this]() { SwitchLayer<SandboxLayerNormalMapping>(); },          // Normal mapping example.
                    [this]() { SwitchLayer<SandboxLayerShadowMapping>(); },          // Shadow mapping example.
                    [this]() { SwitchLayer<SandboxLayerBlinnPhongLighting>(); },     // Blinn-Phong lighting example.
                    [this]() { SwitchLayer<SandboxLayerDepthAndStencilTesting>(); }, // Depth and stencil testing example.
                    [this]() { SwitchLayer<SandboxLayerAttenuation>(); },            // Attenuation example.
                    [this]() { SwitchLayer<SandboxLayerPlanet>(); },                 // Planet rendering example.
                    [this]() { SwitchLayer<SandboxLayerCubes>(); },                  // Cube rendering example.
                    [this]() { SwitchLayer<SandboxLayerPhongLighting>(); },          // Phong lighting example.
                    [this]() { SwitchLayer<SandboxLayerSpecularMap>(); },            // Specular mapping example.
                    [this]() { SwitchLayer<SandboxLayerTerrainGeneration>(); },      // Terrain generation example.
                    [this]() { SwitchLayer<SandboxLayerBackPack>(); },               // Backpack model rendering example.
                    [this]() { SwitchLayer<SandboxLayerSkybox>(); },                 // Skybox rendering example.
                };
            }

            template <typename T> void SwitchLayer() {
                if (app.HasLayer<T>()) {
                    app.ResumeLayer<T>();
                } else {
                    app.PushLayer<T>();
                }
            }

            void SwitchToNextLayer(bool add) {
                // Suspend all known layers
                app.SuspendLayer<SandboxLayerSkybox>();
                app.SuspendLayer<SandboxLayerDeferredShading>();
                app.SuspendLayer<SandboxLayerNormalMapping>();
                app.SuspendLayer<SandboxLayerShadowMapping>();
                app.SuspendLayer<SandboxLayerBlinnPhongLighting>();
                app.SuspendLayer<SandboxLayerFrameBuffer>();
                app.SuspendLayer<SandboxLayerDepthAndStencilTesting>();
                app.SuspendLayer<SandboxLayerAttenuation>();
                app.SuspendLayer<SandboxLayerPlanet>();
                app.SuspendLayer<SandboxLayerCubes>();
                app.SuspendLayer<SandboxLayerPhongLighting>();
                app.SuspendLayer<SandboxLayerSpecularMap>();
                app.SuspendLayer<SandboxLayerTerrainGeneration>();
                app.SuspendLayer<SandboxLayerBackPack>();

                // Move to next/previous layer
                if (add) {
                    currentLayer = (currentLayer + 1) % layerSwitchers.size();
                } else {
                    currentLayer = (currentLayer + layerSwitchers.size() - 1) % layerSwitchers.size();
                }

                // Activate the new layer
                layerSwitchers[currentLayer]();
            }
    };
} // namespace Sandbox
