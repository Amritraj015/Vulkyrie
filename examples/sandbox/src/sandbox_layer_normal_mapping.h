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
                brickWallShader = Shader::Create("assets/shaders/normal_mapping.glsl");
                brickWallTexture = Texture2D::Create("assets/textures/brickwall/brickwall.jpg");
                brickWallNormalMap = Texture2D::Create("assets/textures/brickwall/brickwall_normal.jpg");
                lightShader = Shader::Create("assets/shaders/light-source.glsl");

                // Assert that shader and texture are loaded successfully.
                assert(brickWallShader->IsValid());
                assert(brickWallTexture->IsLoaded());
                assert(brickWallNormalMap->IsLoaded());
                assert(lightShader->IsValid());

                // Create vertex array for the wall.
                brickWallVertexArray = VertexArray::Create();
                Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                    { ShaderDataType::Float2, "aTexCoord" },
                });
                brickWallVertexArray->AddVertexBuffer(vertexBuffer);

                // Create the vertex array for the light source.
                lightSourceVertexArray = VertexArray::Create();
                Ref<VertexBuffer> lightVertexBuffer = VertexBuffer::Create(lightCubeVertices.data(), lightCubeVertices.size() * sizeof(f32));
                lightVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                });
                lightSourceVertexArray->AddVertexBuffer(lightVertexBuffer);

                brickWallShader->Use();

                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                brickWallShader->SetMat4Uniform("projection", projection);

                lightShader->Use();
                lightShader->SetMat4Uniform("projection", projection);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerNormalMapping() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                // clear the color and depth buffer
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Use the graphics shader program.
                brickWallShader->Use();

                // Bind Texture and normal map.
                brickWallTexture->Bind(0);
                brickWallNormalMap->Bind(1);

                // Build transformation matrices and set uniforms.
                brickWallShader->SetVec3Uniform("viewPos", camera.GetPosition());
                brickWallShader->SetMat4Uniform("view", camera.GetViewMatrix());

                auto currentTime = app.GetTime();
                glm::mat4 model = glm::mat4(1.0f);
                brickWallShader->SetMat4Uniform("model", model);

                glm::vec3 lightPos = startingLightPosition;
                lightPos.x = 5.0f * sin(currentTime);
                lightPos.z = 5.0f * cos(currentTime);
                brickWallShader->SetVec3Uniform("lightPos", lightPos);

                // render container
                brickWallVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 6);
                brickWallVertexArray->Unbind();

                lightShader->Use();
                lightShader->SetMat4Uniform("view", camera.GetViewMatrix());
                glm::mat4 lightModelMatrix = glm::mat4(1.0f);
                lightModelMatrix = glm::translate(lightModelMatrix, lightPos);
                lightModelMatrix = glm::scale(lightModelMatrix, glm::vec3(0.2f)); // a smaller cube
                lightShader->SetMat4Uniform("model", lightModelMatrix);

                // Draw the light source.
                lightSourceVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                lightSourceVertexArray->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);
                    return true;
                });
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Normal Mapping");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Normal Mapping");
            }

        private:
            Application &app;
            Camera camera;

            Ref<Texture2D> brickWallTexture;
            Ref<Texture2D> brickWallNormalMap;
            Ref<VertexArray> brickWallVertexArray;
            Ref<Shader> brickWallShader;

            glm::vec3 startingLightPosition;
            Ref<VertexArray> lightSourceVertexArray;
            Ref<Shader> lightShader;
            std::vector<f32> lightCubeVertices = {
                -0.5f, -0.5f, -0.5f, //
                0.5f,  -0.5f, -0.5f, //
                0.5f,  0.5f,  -0.5f, //
                0.5f,  0.5f,  -0.5f, //
                -0.5f, 0.5f,  -0.5f, //
                -0.5f, -0.5f, -0.5f, //

                -0.5f, -0.5f, 0.5f, //
                0.5f,  -0.5f, 0.5f, //
                0.5f,  0.5f,  0.5f, //
                0.5f,  0.5f,  0.5f, //
                -0.5f, 0.5f,  0.5f, //
                -0.5f, -0.5f, 0.5f, //

                -0.5f, 0.5f,  0.5f,  //
                -0.5f, 0.5f,  -0.5f, //
                -0.5f, -0.5f, -0.5f, //
                -0.5f, -0.5f, -0.5f, //
                -0.5f, -0.5f, 0.5f,  //
                -0.5f, 0.5f,  0.5f,  //

                0.5f,  0.5f,  0.5f,  //
                0.5f,  0.5f,  -0.5f, //
                0.5f,  -0.5f, -0.5f, //
                0.5f,  -0.5f, -0.5f, //
                0.5f,  -0.5f, 0.5f,  //
                0.5f,  0.5f,  0.5f,  //

                -0.5f, -0.5f, -0.5f, //
                0.5f,  -0.5f, -0.5f, //
                0.5f,  -0.5f, 0.5f,  //
                0.5f,  -0.5f, 0.5f,  //
                -0.5f, -0.5f, 0.5f,  //
                -0.5f, -0.5f, -0.5f, //

                -0.5f, 0.5f,  -0.5f, //
                0.5f,  0.5f,  -0.5f, //
                0.5f,  0.5f,  0.5f,  //
                0.5f,  0.5f,  0.5f,  //
                -0.5f, 0.5f,  0.5f,  //
                -0.5f, 0.5f,  -0.5f, //
            };

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
