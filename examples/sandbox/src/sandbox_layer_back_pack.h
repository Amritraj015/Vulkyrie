#pragma once

#include <vulkyrie.h>
#include "model.h"

namespace Sandbox {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;

    class SandboxLayerBackPack final : public Layer {
        public:
            SandboxLayerBackPack(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application), camera(glm::vec3(0.0f, 0.0f, 8.0f)), windowWidth(windowWidth), windowHeight(windowHeight) {

                backPackModel = CreateRef<Model>("assets/models/backpack/backpack.obj");
                graphicsShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/model.glsl");

                if (!graphicsShader->IsValid()) {
                    VERROR("Failed to compile shaders.")
                }

                // Enable depth testing for proper 3D rendering.
                glEnable(GL_DEPTH_TEST);

                // Enable face culling to improve performance.
                // glEnable(GL_CULL_FACE);
            };

            ~SandboxLayerBackPack() = default;

            void OnAttached() override { VDEBUG("Layer Attached: Backpack", _id.GetUUID()) };

            void OnDetached() override { VDEBUG("Layer Detached: Backpack") };

            void OnUpdate(const Vulkyrie::Core::Timestep deltaTime) override {
                glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                float cameraSpeed = 5.0f;
                auto dt = deltaTime.GetSeconds();

                if (_application.IsKeyPressed(KeyCode::LeftShift)) cameraSpeed = 20.0f;

                if (_application.IsKeyPressed(KeyCode::W)) camera.ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::S)) camera.ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::A)) camera.ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::D)) camera.ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);

                graphicsShader->Use();

                // view/projection transformations
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
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
                    auto scrollEvent = static_cast<MouseScrolledEvent>(e);
                    camera.ProcessMouseScroll(scrollEvent.OffsetY);

                    return true;
                });

                dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](const Vulkyrie::Events::WindowResizedEvent &e) {
                    auto ev = static_cast<Vulkyrie::Events::WindowResizedEvent>(e);
                    glm::mat4 projection = glm::mat4(1.0f);
                    projection = glm::perspective(glm::radians(45.0f), (float)ev.Width / (float)ev.Height, 0.1f, 100.0f);

                    return true;
                });

                dispatcher.Dispatch<Vulkyrie::Events::MouseMovedEvent>([this](const Vulkyrie::Events::MouseMovedEvent &e) {
                    auto mouseMovedEvent = static_cast<Vulkyrie::Events::MouseMovedEvent>(e);

                    if (firstMouseMove) {
                        lastMouseX = mouseMovedEvent.MouseX;
                        lastMouseY = mouseMovedEvent.MouseY;
                        firstMouseMove = false;
                    }

                    const f32 xOffset = mouseMovedEvent.MouseX - lastMouseX;
                    const f32 yOffset = lastMouseY - mouseMovedEvent.MouseY;

                    camera.ProcessMouseMovement(xOffset, yOffset);

                    lastMouseX = mouseMovedEvent.MouseX;
                    lastMouseY = mouseMovedEvent.MouseY;

                    return true;
                });
            };

        private:
            Ref<Model> backPackModel;
            Ref<Shader> graphicsShader;
            Camera camera;
            f64 lastMouseX = 400.0f;
            f64 lastMouseY = 300.0f;
            bool firstMouseMove = true;
            f32 windowHeight, windowWidth;
    };
} // namespace Sandbox
