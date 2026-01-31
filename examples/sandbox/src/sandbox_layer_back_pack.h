#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;

    class SandboxLayerBackPack final : public Layer {
        public:
            SandboxLayerBackPack()
                : camera(glm::vec3(0.0f, 0.0f, 8.0f)) {

                backPackModel = Model::Create("assets/models/backpack/backpack.obj");
                graphicsShader = Shader::Create("assets/shaders/model.glsl");

                if (!graphicsShader->IsValid()) {
                    VERROR("Failed to compile shaders.")
                }

                // Enable depth testing for proper 3D rendering.
                glEnable(GL_DEPTH_TEST);

                // Enable face culling to improve performance.
                // glEnable(GL_CULL_FACE);
            };

            ~SandboxLayerBackPack() = default;

            void OnAttached() override { VDEBUG("Layer Attached: Backpack") };

            void OnDetached() override { VDEBUG("Layer Detached: Backpack") };

            void OnUpdate(const Timestep &deltaTime) override {
                glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                camera.OnUpdate(deltaTime);

                graphicsShader->Use();

                // view/projection transformations
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        1000.0f);
                glm::mat4 view = camera.GetViewMatrix();
                graphicsShader->SetMat4Uniform("projection", projection);
                graphicsShader->SetMat4Uniform("view", view);

                // render the loaded model
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));     // it's a bit too big for our scene, so scale it down
                graphicsShader->SetMat4Uniform("model", model);

                backPackModel->Draw(*graphicsShader);
            };

            void OnEvent(Vulkyrie::Events::Event &event) override {
                Vulkyrie::Events::EventDispatcher dispatcher(event);

                dispatcher.Dispatch<Vulkyrie::Events::MouseScrolledEvent>([this](const Vulkyrie::Events::MouseScrolledEvent &e) {
                    camera.ProcessMouseScroll(e.OffsetY);

                    return true;
                });
            };

        private:
            Ref<Model> backPackModel;
            Ref<Shader> graphicsShader;
            Camera camera;
    };
} // namespace Sandbox
