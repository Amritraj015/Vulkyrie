#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie;

    class SandboxLayerPhongLighting final : public Layer {
        public:
            SandboxLayerPhongLighting()
                : app(Application::GetSingleton())
                , camera(Camera::Create()) {
                // Load and compile shader programs.
                objectShader = Shader::Create("assets/shaders/reflective-object.glsl");
                lightShader = Shader::Create("assets/shaders/light-source.glsl");

                // Assert that shaders are loaded successfully.
                assert(objectShader->IsValid());
                assert(lightShader->IsValid());

                // Create Vertex Array.
                objectVertexArray = VertexArray::Create();

                // Create Vertex Buffer.
                auto objectVertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));

                // Set layout for the vertex buffer.
                objectVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                    { ShaderDataType::Float3, "aNormal" },
                });

                // Add Vertex Buffer to the vertex array.
                objectVertexArray->AddVertexBuffer(objectVertexBuffer);

                // Create the vertex array for the light source.
                lightVertexArray = VertexArray::Create();

                // Reuse the same vertex buffer for the light source.
                lightVertexArray->AddVertexBuffer(objectVertexBuffer);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerPhongLighting() = default;

            void OnAttached() override {
                VDEBUG("Layer Attached: Phong Lighting");
            }
            void OnDetached() override {
                VDEBUG("Layer Detached: Phong Lighting");
            }

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                objectShader->Use();
                objectShader->SetVec3Uniform("viewPos", camera.GetPosition());

                // view/projection transformations
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        1000.0f);
                glm::mat4 view = camera.GetViewMatrix();
                objectShader->SetMat4Uniform("projection", projection);
                objectShader->SetMat4Uniform("view", view);

                // world transformation
                glm::mat4 model = glm::mat4(1.0f);
                objectShader->SetMat4Uniform("model", model);

                // Set the material properties for the object.
                objectShader->SetVec3Uniform("material.ambient", 1.0f, 0.5f, 0.31f);
                objectShader->SetVec3Uniform("material.diffuse", 1.0f, 0.5f, 0.31f);
                objectShader->SetVec3Uniform("material.specular", 0.5f, 0.5f, 0.5f);
                objectShader->SetFloatUniform("material.shininess", 32.0f);

                lightPos = glm::vec3(1.0f + sin(glfwGetTime()) * 2.0f, sin(glfwGetTime() / 2.0f) * 1.0f, 2.0f);

                // Set the light properties.
                objectShader->SetVec3Uniform("light.position", lightPos);
                objectShader->SetVec3Uniform("light.specular", 1.0f, 1.0f, 1.0f);

                // Change ambient and diffuse light color over time.
                glm::vec3 lightColor;
                f32 currentTime = app.GetTime();
                lightColor.x = sin(currentTime * 2.0f);
                lightColor.y = sin(currentTime * 0.7f);
                lightColor.z = sin(currentTime * 1.3f);

                glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);   // decrease the influence
                glm::vec3 ambientColor = diffuseColor * glm::vec3(0.3f); // low influence

                objectShader->SetVec3Uniform("light.ambient", ambientColor);
                objectShader->SetVec3Uniform("light.diffuse", diffuseColor); // darken diffuse light a bit

                // Issue a draw call to draw the light source.
                objectVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // -----------------------------------------------------------------------------------
                // also draw the lamp object
                lightShader->Use();
                lightShader->SetMat4Uniform("projection", projection);
                lightShader->SetMat4Uniform("view", view);
                model = glm::translate(model, lightPos);
                model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
                lightShader->SetMat4Uniform("model", model);

                // graphicsShader.Use();
                lightVertexArray->Bind();

                // Issue a draw call to draw the reflecting object.
                glDrawArrays(GL_TRIANGLES, 0, 36);
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
            Camera camera;

            Ref<VertexArray> objectVertexArray;
            Ref<Shader> objectShader;

            Ref<VertexArray> lightVertexArray;
            Ref<Shader> lightShader;

            glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);

            std::vector<f32> vertices = {
                -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, //
                0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, //
                0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, //

                -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, //

                -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f, //
                -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f, //
                -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f, //
                -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f, //
                -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f, //
                -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f, //

                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, //

                -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, //

                -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f, //
                0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f  //
            };
    };
} // namespace Sandbox
