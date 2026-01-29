#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;

    class SandboxLayerSkybox final : public Vulkyrie::Core::Layer {
        public:
            SandboxLayerSkybox(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application)
                , windowWidth(windowWidth)
                , windowHeight(windowHeight)
                , camera(glm::vec3(0.0f, 0.0f, 5.0f))
                , texture(Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/wall.jpg"))
                , terrainShader(Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/triangle.glsl")) {
                if (!texture->IsLoaded()) {
                    VERROR("SandboxLayerSkybox: Failed to load texture.");
                    return;
                }

                if (!terrainShader->IsValid()) {
                    VERROR("SandboxLayerSkybox: Failed to load shaders.");
                    return;
                }

                vertexArray = VertexArray::Create(GraphicsAPI::OpenGL);

                Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, vertices.data(), vertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                    { ShaderDataType::Float2, "aTexCoord" },
                });
                vertexArray->AddVertexBuffer(vertexBuffer);

                Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(GraphicsAPI::OpenGL, indices.data(), indices.size());
                vertexArray->SetIndexBuffer(indexBuffer);

                // Load sky-box cubemap textures.
                skyboxTexture = TextureCubeMap::Create(GraphicsAPI::OpenGL,
                                                       {
                                                           "assets/cubemaps/skybox/right.jpg",
                                                           "assets/cubemaps/skybox/left.jpg",
                                                           "assets/cubemaps/skybox/top.jpg",
                                                           "assets/cubemaps/skybox/bottom.jpg",
                                                           "assets/cubemaps/skybox/front.jpg",
                                                           "assets/cubemaps/skybox/back.jpg",
                                                       });

                if (!skyboxTexture->IsValid()) {
                    VERROR("SandboxLayerSkybox: Failed to load skybox cubemap texture.");
                    return;
                }

                skyboxVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);
                Ref<VertexBuffer> skyboxVertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, skyboxVertices.data(), skyboxVertices.size() * sizeof(f32));
                skyboxVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                });
                skyboxVertexArray->AddVertexBuffer(skyboxVertexBuffer);

                // Load skybox shaders.
                skyboxShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/skybox.glsl");

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
            }

            void OnUpdate(const Timestep &deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                camera.OnUpdate(deltaTime);
                // --------------------------------------------------------------------
                // Render terrain.
                terrainShader->Use();

                // Projection Matrix.
                glm::mat4 projection = glm::mat4(1.0f);
                projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);
                terrainShader->SetMat4Uniform("projection", projection);

                // View Matrix.
                glm::mat4 viewTerrain = camera.GetViewMatrix();
                terrainShader->SetMat4Uniform("view", viewTerrain);

                texture->Bind(0);

                vertexArray->Bind();

                for (int i = 0; i < HEIGHT; i++) {
                    const auto x = i * WIDTH;

                    for (int j = 0; j < WIDTH; j++) {
                        glm::mat4 model(1.0f);

                        glm::vec3 firstQuadrantPosition = glm::vec3(j * -1.0f, -1.0f, i * -1.0f);
                        model = glm::translate(model, firstQuadrantPosition);
                        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        terrainShader->SetMat4Uniform("model", model);

                        glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
                    }
                }

                vertexArray->Unbind();
                // --------------------------------------------------------------------

                // --------------------------------------------------------------------
                // Sky box.
                // change depth function so depth test passes when values are equal to depth buffer's content
                glDepthFunc(GL_LEQUAL);
                skyboxShader->Use();

                glm::mat4 view = camera.GetViewMatrix();
                view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
                skyboxShader->SetMat4Uniform("projection", projection);
                skyboxShader->SetMat4Uniform("view", view);

                skyboxTexture->Bind();

                skyboxVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                skyboxVertexArray->Unbind();

                // set depth function back to default
                glDepthFunc(GL_LESS);
                // --------------------------------------------------------------------
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Skybox");
            }

            void OnDetached() override {
                VDEBUG("Layer Attached: Skybox");
            }

            void OnEvent(Vulkyrie::Events::Event &event) override {
                Vulkyrie::Events::EventDispatcher dispatcher(event);

                dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](const Vulkyrie::Events::WindowResizedEvent &e) {
                    glm::mat4 projection = glm::mat4(1.0f);
                    projection = glm::perspective(glm::radians(45.0f), (f32)e.Width / (f32)e.Height, 0.1f, 100.0f);

                    terrainShader->SetMat4Uniform("projection", projection);

                    return true;
                });
            }

        private:
            Camera camera;
            f32 windowWidth;
            f32 windowHeight;

            Ref<Shader> terrainShader;
            Ref<VertexArray> vertexArray;
            Ref<Texture2D> texture;

            Ref<Shader> skyboxShader;
            Ref<VertexArray> skyboxVertexArray;
            Ref<TextureCubeMap> skyboxTexture;

            std::vector<f32> vertices = {
                // positions        // texture coords
                0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // top right
                0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, // bottom right
                -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left
                -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, // top left
            };

            std::vector<u32> indices = {
                0, 1, 2, // first triangle
                0, 2, 3  // second triangle
            };

            std::vector<f32> skyboxVertices = {
                // positions
                -1.0f, 1.0f,  -1.0f, //
                -1.0f, -1.0f, -1.0f, //
                1.0f,  -1.0f, -1.0f, //
                1.0f,  -1.0f, -1.0f, //
                1.0f,  1.0f,  -1.0f, //
                -1.0f, 1.0f,  -1.0f, //

                -1.0f, -1.0f, 1.0f,  //
                -1.0f, -1.0f, -1.0f, //
                -1.0f, 1.0f,  -1.0f, //
                -1.0f, 1.0f,  -1.0f, //
                -1.0f, 1.0f,  1.0f,  //
                -1.0f, -1.0f, 1.0f,  //

                1.0f,  -1.0f, -1.0f, //
                1.0f,  -1.0f, 1.0f,  //
                1.0f,  1.0f,  1.0f,  //
                1.0f,  1.0f,  1.0f,  //
                1.0f,  1.0f,  -1.0f, //
                1.0f,  -1.0f, -1.0f, //

                -1.0f, -1.0f, 1.0f, //
                -1.0f, 1.0f,  1.0f, //
                1.0f,  1.0f,  1.0f, //
                1.0f,  1.0f,  1.0f, //
                1.0f,  -1.0f, 1.0f, //
                -1.0f, -1.0f, 1.0f, //

                -1.0f, 1.0f,  -1.0f, //
                1.0f,  1.0f,  -1.0f, //
                1.0f,  1.0f,  1.0f,  //
                1.0f,  1.0f,  1.0f,  //
                -1.0f, 1.0f,  1.0f,  //
                -1.0f, 1.0f,  -1.0f, //

                -1.0f, -1.0f, -1.0f, //
                -1.0f, -1.0f, 1.0f,  //
                1.0f,  -1.0f, -1.0f, //
                1.0f,  -1.0f, -1.0f, //
                -1.0f, -1.0f, 1.0f,  //
                1.0f,  -1.0f, 1.0f,  //
            };

            static constexpr u16 WIDTH = 100;
            static constexpr u16 HEIGHT = 100;
    };
} // namespace Sandbox
