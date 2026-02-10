#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;

    class SandboxLayerNormalMapping final : public Layer {
        public:
            SandboxLayerNormalMapping()
                : app(Application::GetSingleton())
                , camera(Camera::Create())
                , startingLightPosition(0.0f, 0.0f, 3.0f) {

                // Load shaders and textures.
                shader = Shader::Create("assets/shaders/normal_mapping.glsl");
                brickWallTexture = Texture2D::Create("assets/textures/brickwall/brickwall.jpg");
                brickWallNormalMap = Texture2D::Create("assets/textures/brickwall/brickwall_normal.jpg");

                // Assert that shader and texture are loaded successfully.
                assert(shader->IsValid());
                assert(brickWallTexture->IsLoaded());
                assert(brickWallNormalMap->IsLoaded());

                // Create vertex array for the surface.
                vertexArray = VertexArray::Create();
                Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                    { ShaderDataType::Float2, "aTexCoord" },
                });
                vertexArray->AddVertexBuffer(vertexBuffer);

                shader->Use();

                // Bind Texture and normal map.
                brickWallTexture->Bind(0);
                brickWallNormalMap->Bind(1);

                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                shader->SetMat4Uniform("projection", projection);

                glm::mat4 model = glm::mat4(1.0f);
                shader->SetMat4Uniform("model", model);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerNormalMapping() = default;

            void OnAttached() override {
                VDEBUG("Layer Attached: Normal Mapping");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Normal Mapping");
            }

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                // clear the color and depth buffer
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Use the graphics shader program.
                shader->Use();

                // Build transformation matrices and set uniforms.
                shader->SetVec3Uniform("viewPos", camera.GetPosition());
                shader->SetMat4Uniform("view", camera.GetViewMatrix());
                glm::vec3 lightPos = startingLightPosition;
                auto currentTime = app.GetTime();
                lightPos.x = 1.5f * sin(currentTime);
                lightPos.z = 1.5f * cos(currentTime);
                shader->SetVec3Uniform("lightPos", lightPos);

                // render container
                vertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 6);
                vertexArray->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });
            }

        private:
            Application &app;
            Ref<Texture2D> brickWallTexture;
            Ref<Texture2D> brickWallNormalMap;
            Ref<VertexArray> vertexArray;
            Ref<Shader> shader;
            glm::vec3 startingLightPosition;
            Camera camera;

            std::vector<f32> vertices = {
                // positions        // texture coords
                1.0f,  1.0f,  0.0f, 1.0f, 1.0f, //
                1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, //
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, //

                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, //
                -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, //
                1.0f,  1.0f,  0.0f, 1.0f, 1.0f, //
            };
    };
} // namespace Sandbox
