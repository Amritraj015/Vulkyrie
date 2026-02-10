#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;

    class SandboxLayerCubes final : public Vulkyrie::Core::Layer {
        public:
            SandboxLayerCubes()
                : camera(Camera::Create()) {

                // Load and compile shader program.
                graphicsShader = Shader::Create("assets/shaders/triangle.glsl");

                // Check if shader program creation failed.
                if (!graphicsShader->IsValid()) {
                    // Log a fatal error.
                    VFATAL("Failed to create graphics shader");

                    return;
                }

                vertexArray = VertexArray::Create();
                Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                    { ShaderDataType::Float2, "aTexCoord" },
                });
                vertexArray->AddVertexBuffer(vertexBuffer);

                // -----------------------------------------------
                // Textures.
                texture1 = Texture2D::Create("assets/textures/wall.jpg");
                texture2 = Texture2D::Create("assets/textures/awesomeface.png");

                if (!texture1->IsLoaded() || !texture2->IsLoaded()) {
                    VERROR("Failed to load one or more textures!");
                }

                // Projection matrix hardly ever changes, so it can live outside the main application loop.
                graphicsShader->Use();
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        1000.0f);

                graphicsShader->SetMat4Uniform("projection", projection);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);

                // Enable face culling to improve performance.
                // glDisable(GL_CULL_FACE);
            }

            ~SandboxLayerCubes() = default;

            void OnAttached() override {
                VDEBUG("Layer Attached: Cubes");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Cubes");
            }

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                // clear the color and depth buffer
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Use the graphics shader program.
                graphicsShader->Use();

                // bind Texture
                texture1->Bind(0);
                texture2->Bind(1);

                auto view = camera.GetViewMatrix();

                graphicsShader->SetMat4Uniform("view", view);

                // render container
                vertexArray->Bind();

                for (u32 i = 0; i < cubePositions.size(); i++) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, cubePositions[i]);
                    f32 angle = 20.0f * (i + 1);

                    model = glm::rotate(model, (f32)glfwGetTime() * glm::radians(angle), glm::vec3(0.5f, 1.0f, 0.0f));
                    graphicsShader->SetMat4Uniform("model", model);

                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

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
            Ref<Texture2D> texture1;
            Ref<Texture2D> texture2;
            Ref<VertexArray> vertexArray;
            Ref<Shader> graphicsShader;
            Camera camera;

            std::vector<glm::vec3> cubePositions = {
                glm::vec3(0.0f, 0.0f, 0.0f),     // Cube 1
                glm::vec3(2.0f, 5.0f, -15.0f),   // Cube 2
                glm::vec3(-1.5f, -2.2f, -2.5f),  // Cube 3
                glm::vec3(-3.8f, -2.0f, -12.3f), // Cube 4
                glm::vec3(2.4f, -0.4f, -3.5f),   // Cube 5
                glm::vec3(-1.7f, 3.0f, -7.5f),   // Cube 6
                glm::vec3(1.3f, -2.0f, -2.5f),   // Cube 7
                glm::vec3(1.5f, 2.0f, -2.5f),    // Cube 8
                glm::vec3(1.5f, 0.2f, -1.5f),    // Cube 9
                glm::vec3(-1.3f, 1.0f, -1.5f)    // Cube 10
            };

            std::vector<f32> vertices = {
                -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f, 0.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, //

                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 1.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 1.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //

                -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, //
                -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //
                -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, //

                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //

                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f, 1.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //

                -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, //
            };
    };
} // namespace Sandbox
