#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;

    class SandboxLayerBlinnPhongLighting final : public Layer {
        public:
            SandboxLayerBlinnPhongLighting()
                : app(Application::GetSingleton())
                , camera(Camera::Create())
                , useBlinnPhong(true)
                , lightPosition(3.0f, 4.0f, 3.0f) {

                camera.SetMovementSpeed(1.0f, 5.0f, 20.0f);

                // Load shaders and textures.
                shader = Shader::Create("assets/shaders/blinn_phong.glsl");
                surfaceTexture = Texture2D::Create("assets/textures/wood.png");

                // Assert that shader and texture are loaded successfully.
                assert(shader->IsValid());
                assert(surfaceTexture->IsLoaded());

                // Create vertex array for the surface.
                vertexArray = VertexArray::Create();
                Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(surfaceVertices.data(), surfaceVertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float3, "normal" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                vertexArray->AddVertexBuffer(vertexBuffer);

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerBlinnPhongLighting() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Use shader.
                shader->Use();

                // View Matrix.
                shader->SetMat4Uniform("view", camera.GetViewMatrix());

                // Set other uniforms.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                shader->SetMat4Uniform("projection", projection);
                shader->SetMat4Uniform("model", glm::mat4(1.0f));
                shader->SetVec3Uniform("viewPos", camera.GetPosition());
                auto currentTime = app.GetTime();
                shader->SetVec3Uniform("lightPos", glm::vec3(lightPosition.x * sin(currentTime), lightPosition.y, lightPosition.z * cos(currentTime)));
                shader->SetBoolUniform("useBlinnPhong", useBlinnPhong);

                // Draw surface and light.
                vertexArray->Bind();
                surfaceTexture->Bind(0);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                vertexArray->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
                    if (e.KeyCode == KeyCode::U) {
                        useBlinnPhong = !useBlinnPhong;
                        return true;
                    }

                    if (e.KeyCode == KeyCode::I) {
                        lightPosition.y += 0.5f;
                        return true;
                    }

                    if (e.KeyCode == KeyCode::O) {
                        lightPosition.y -= 0.5f;
                        return true;
                    }

                    if (e.KeyCode == KeyCode::R) {
                        shader->Reload();
                        return true;
                    }

                    return false;
                });
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Blinn-Phong Lighting");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Blinn-Phong Lighting");
            }

        private:
            Application &app;
            Camera camera;
            Ref<Texture2D> surfaceTexture;
            Ref<VertexArray> vertexArray;
            Ref<Shader> shader;
            bool useBlinnPhong;
            glm::vec3 lightPosition;
            std::vector<f32> surfaceVertices = {
                // positions           // normals        // texcoords
                10.0f,  -0.5f, 10.0f,  0.0f, 1.0f, 0.0f, 10.0f, 0.0f,  //
                -10.0f, -0.5f, 10.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f,  //
                -10.0f, -0.5f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f,  10.0f, //

                10.0f,  -0.5f, 10.0f,  0.0f, 1.0f, 0.0f, 10.0f, 0.0f,  //
                -10.0f, -0.5f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f,  10.0f, //
                10.0f,  -0.5f, -10.0f, 0.0f, 1.0f, 0.0f, 10.0f, 10.0f, //
            };
    };
} // namespace Sandbox
