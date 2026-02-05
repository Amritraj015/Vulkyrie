#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;

    class SandboxLayerDeferredShading final : public Layer {
        public:
            SandboxLayerDeferredShading()
                : camera(Camera::Create()) {

                // Load and compile shader program.
                shader = Shader::Create("assets/shaders/triangle.glsl");
                backPackModel = Model::Create("assets/models/backpack/backpack.obj");

                // Check if shader program creation failed.
                if (!shader->IsValid()) {
                    // Log a fatal error.
                    VFATAL("Failed to create graphics shader");

                    return;
                }

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerDeferredShading() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                // clear the color and depth buffer
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Use the graphics shader program.
                shader->Use();

                auto view = camera.GetViewMatrix();
                shader->SetMat4Uniform("view", view);

                for (u32 i = 0; i < backpackLocations.size(); i++) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, backpackLocations[i]);
                }
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: DeferredShading");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: DeferredShading");
            }

        private:
            Ref<Model> backPackModel;
            Ref<Shader> shader;
            Camera camera;

            std::vector<glm::vec3> backpackLocations = {
                glm::vec3(-3.0, -0.5, -3.0), //
                glm::vec3(0.0, -0.5, -3.0),  //
                glm::vec3(3.0, -0.5, -3.0),  //
                glm::vec3(-3.0, -0.5, 0.0),  //
                glm::vec3(0.0, -0.5, 0.0),   //
                glm::vec3(3.0, -0.5, 0.0),   //
                glm::vec3(-3.0, -0.5, 3.0),  //
                glm::vec3(0.0, -0.5, 3.0),   //
                glm::vec3(3.0, -0.5, 3.0),   //
            };
    };
} // namespace Sandbox
