#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;

    class SandboxLayerPhongLighting final : public Vulkyrie::Core::Layer {
        public:
            SandboxLayerPhongLighting()
                : camera(glm::vec3(0.0f, 0.0f, 5.0f)) {
                // Load and compile shader programs.
                objectShader = Shader::Create("assets/shaders/reflective-object.glsl");
                lightShader = Shader::Create("assets/shaders/light-source.glsl");

                // Check if shaders are valid.
                if (!objectShader->IsValid() || !lightShader->IsValid()) {
                    VERROR("Failed to load shaders.");
                    return;
                }

                // Create Vertex Array.
                objectVertexArray = VertexArray::Create();

                // Create Vertex Buffer.
                objectVertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));

                // Set layout for the vertex buffer.
                objectVertexBuffer->SetLayout({
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aPos" },
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aNormal" },
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

            void OnAttached() override { VDEBUG("Layer Attached: Phong Lighting"); }
            void OnDetached() override { VDEBUG("Layer Detached: Phong Lighting"); }

            void OnUpdate(const Timestep &deltaTime) override {
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
                f64 currentTime = glfwGetTime();
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

        private:
            Ref<VertexArray> objectVertexArray;
            Ref<VertexBuffer> objectVertexBuffer;
            Ref<Shader> objectShader;

            Ref<VertexArray> lightVertexArray;
            Ref<VertexBuffer> lightVertexBuffer;
            Ref<Shader> lightShader;

            Camera camera;

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
