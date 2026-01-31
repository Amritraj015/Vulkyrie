#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;

    class SandboxLayerSpecularMap : public Layer {
        public:
            SandboxLayerSpecularMap()
                : camera(glm::vec3(0.0f, 0.0f, 5.0f)) {

                // Initial light position.
                lightPos = glm::vec3(1.2f, 1.0f, 2.0f);

                // load and compile the shader programs.
                objectShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/specular-highlight.glsl");
                lightShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/light-source.glsl");

                // Check if shaders are loaded successfully.
                if (!objectShader->IsValid() || !lightShader->IsValid()) {
                    VERROR("Failed to load shaders.");
                    return;
                }

                // load the textures.
                boxTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/container2.png");
                specularMapTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/container2_specular.png");

                // Check if textures are loaded successfully.
                if (!boxTexture->IsLoaded() || !specularMapTexture->IsLoaded()) {
                    VERROR("Failed to load one or more textures!");
                    return;
                }

                // Create Vertex Array.
                objectVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);

                // Create Vertex Buffer.
                objectVertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, vertices.data(), vertices.size() * sizeof(f32));

                // Set layout for the vertex buffer.
                objectVertexBuffer->SetLayout({
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aPos" },
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aNormal" },
                    { Vulkyrie::Renderer::ShaderDataType::Float2, "aTexture" },
                });

                // Add Vertex Buffer to the vertex array.
                objectVertexArray->AddVertexBuffer(objectVertexBuffer);

                // Create the vertex array for the light source.
                lightVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);

                // Reuse the same vertex buffer for the light source.
                lightVertexArray->AddVertexBuffer(objectVertexBuffer);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerSpecularMap() = default;

            void OnAttached() override { VDEBUG("Layer Attached: Specular Map"); }
            void OnDetached() override { VDEBUG("Layer Detached: Specular Map"); }

            void OnUpdate(const Timestep &deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                objectShader->Use();
                objectShader->SetVec3Uniform("viewPos", camera.GetPosition());

                // projection transformations.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        1000.0f);
                objectShader->SetMat4Uniform("projection", projection);

                // view matrix.
                glm::mat4 view = camera.GetViewMatrix();
                objectShader->SetMat4Uniform("view", view);

                // world transformation.
                glm::mat4 model = glm::mat4(1.0f);
                objectShader->SetMat4Uniform("model", model);

                // Set the material properties for the object.
                objectShader->SetIntUniform("material.diffuse", 0);
                objectShader->SetIntUniform("material.specular", 1);
                objectShader->SetFloatUniform("material.shininess", 32.0f);

                // Bind the textures to texture units.
                boxTexture->Bind(0);
                specularMapTexture->Bind(1);

                // Set the light properties.
                objectShader->SetVec3Uniform("light.position", lightPos);
                objectShader->SetVec3Uniform("light.ambient", 0.2f, 0.2f, 0.2f);
                objectShader->SetVec3Uniform("light.diffuse", 0.5f, 0.5f, 0.5f);
                objectShader->SetVec3Uniform("light.specular", 1.0f, 1.0f, 1.0f);

                // Issue a draw call to draw the reflecting object.
                objectVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                objectVertexArray->Unbind();

                // -----------------------------------------------------------------------------------
                // also draw the lamp object
                lightShader->Use();
                lightShader->SetMat4Uniform("projection", projection);
                lightShader->SetMat4Uniform("view", view);
                model = glm::translate(model, lightPos);
                model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
                lightShader->SetMat4Uniform("model", model);

                // Issue a draw call to draw the light source.
                lightVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                lightVertexArray->Unbind();
            }

        private:
            Ref<VertexArray> objectVertexArray;
            Ref<VertexBuffer> objectVertexBuffer;
            Ref<Shader> objectShader;

            Ref<VertexArray> lightVertexArray;
            Ref<VertexBuffer> lightVertexBuffer;
            Ref<Shader> lightShader;

            glm::vec3 lightPos;

            Ref<Texture2D> boxTexture;
            Ref<Texture2D> specularMapTexture;

            Camera camera;

            std::vector<f32> vertices = {
                // positions         // normals          // texture coords
                -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 0.0f, //
                0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f, //
                0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, //

                -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, //

                -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, //
                -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,  1.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, //
                -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f, 0.0f, //
                -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, //

                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,  1.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, //

                -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f, 1.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f, 0.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, //

                -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f, //
                0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  1.0f, 1.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f  //
            };
    };
} // namespace Sandbox
