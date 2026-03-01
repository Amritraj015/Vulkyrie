#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;

    class SandboxLayerTerrainGeneration final : public Layer {
        public:
            SandboxLayerTerrainGeneration()
                : app(Application::GetSingleton())
                , camera(Camera::Create()) {

                // Load and compile shader program.
                terrainShader = Shader::Create("assets/shaders/terrain.glsl");

                // Assert that shader is loaded successfully.
                assert(terrainShader->IsValid());

                vertices.reserve(width * height * 3);
                CreateVertexBufferElements();

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
            }

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                camera.OnUpdate(deltaTime);

                // Render terrain.
                terrainShader->Use();

                // Projection Matrix.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                terrainShader->SetMat4Uniform("projection", projection);

                // View Matrix.
                glm::mat4 view = camera.GetViewMatrix();
                terrainShader->SetMat4Uniform("view", view);

                // Model Matrix.
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-50.0f, -25.0f, -115.0f));
                terrainShader->SetMat4Uniform("model", model);

                vertexArray->Bind();
                glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
                vertexArray->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);
                    return true;
                });

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
                    if (e.KeyCode == KeyCode::M) {
                        if (scale < 100.0f) scale += 1.0f;
                        CreateVertexBufferElements();
                    }

                    if (e.KeyCode == KeyCode::N) {
                        if (scale > 2.0f) scale -= 1.0f;
                        CreateVertexBufferElements();
                    }

                    if (e.KeyCode == KeyCode::Y) {
                        seed += 1.0f;
                        CreateVertexBufferElements();
                    }

                    if (e.KeyCode == KeyCode::U) {
                        persistence += 0.001f;
                        CreateVertexBufferElements();
                    }

                    if (e.KeyCode == KeyCode::H) {
                        offset = glm::vec2(offset.x - 0.01f, offset.y);
                        CreateVertexBufferElements();
                    }

                    if (e.KeyCode == KeyCode::L) {
                        offset = glm::vec2(offset.x, offset.y + 0.01f);
                        CreateVertexBufferElements();
                    }

                    if (e.KeyCode == KeyCode::I) {
                        lacunarity += 0.1f;
                        CreateVertexBufferElements();
                    }

                    if (e.KeyCode == KeyCode::O) {
                        octaves += 1.0f;
                        CreateVertexBufferElements();
                    }

                    return false;
                });
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Terrain Generation");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Terrain Generation");
            }

        private:
            Application &app;
            Camera camera;
            Ref<Shader> terrainShader;
            Ref<VertexArray> vertexArray;
            std::vector<f32> noiseMap;
            std::vector<f32> vertices;
            std::vector<u32> indices;

            u16 width = 100;
            u16 height = 100;
            f32 worldSizeX = 100.0f; // meters wide
            f32 worldSizeZ = 100.0f; // meters deep
            f32 heightScale = 15.0f; // max height in meters

            f32 scale = 5.0f;
            i32 octaves = 4;
            f32 persistence = 0.5f;
            f32 lacunarity = 2.0f;
            u32 seed = 42;
            glm::vec2 offset = glm::vec2(10.0f, 10.0f);

            void CreateVertexBufferElements() {
                noiseMap = Vulkyrie::Core::GeneratePerlinNoiseMap({
                    .MapWidth = width,
                    .MapHeight = height,
                    .Scale = scale,
                    .Octaves = octaves,
                    .Persistence = persistence,
                    .Lacunarity = lacunarity,
                    .Seed = seed,
                    .Offset = offset,
                });

                vertices.clear();

                for (size_t z = 0; z < height; ++z) {
                    for (size_t x = 0; x < width; ++x) {
                        f32 h = noiseMap[z * width + x];

                        f32 worldX = (f32(x) / (width - 1)) * worldSizeX;
                        f32 worldZ = (f32(z) / (height - 1)) * worldSizeZ;
                        f32 worldY = h * heightScale;

                        vertices.emplace_back(worldX);
                        vertices.emplace_back(worldY);
                        vertices.emplace_back(worldZ);

                        if (h > 0.85f) {
                            vertices.emplace_back(1.0f);
                            vertices.emplace_back(1.0f);
                            vertices.emplace_back(1.0f);
                        } else if (h > 0.7f) {
                            vertices.emplace_back(0.588f);
                            vertices.emplace_back(0.294f);
                            vertices.emplace_back(0.0f);
                        } else if (h > 0.4f) {
                            vertices.emplace_back(0.0f);
                            vertices.emplace_back(0.5f);
                            vertices.emplace_back(0.0f);
                        } else {
                            vertices.emplace_back(0.0f);
                            vertices.emplace_back(0.0f);
                            vertices.emplace_back(1.0f);
                        }
                    }
                }

                if (!vertexArray) {
                    for (size_t z = 0; z < height - 1; ++z) {
                        for (size_t x = 0; x < width - 1; ++x) {
                            u32 i0 = z * width + x;
                            u32 i1 = z * width + x + 1;
                            u32 i2 = (z + 1) * width + x;
                            u32 i3 = (z + 1) * width + x + 1;

                            // Triangle 1
                            indices.push_back(i0);
                            indices.push_back(i2);
                            indices.push_back(i1);

                            // Triangle 2
                            indices.push_back(i1);
                            indices.push_back(i2);
                            indices.push_back(i3);
                        }
                    }

                    vertexArray = VertexArray::Create();

                    Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices.size() * sizeof(f32));
                    vertexBuffer->SetLayout({
                        { ShaderDataType::Float3, "position" },
                        { ShaderDataType::Float3, "color" },
                    });
                    vertexArray->AddVertexBuffer(vertexBuffer);
                    vertexArray->GetVertexBuffer(0).SetData(vertices.data(), vertices.size() * sizeof(f32));

                    Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size());
                    vertexArray->SetIndexBuffer(indexBuffer);
                } else {
                    vertexArray->GetVertexBuffer(0).SetData(vertices.data(), vertices.size() * sizeof(f32));
                }
            }
    };
} // namespace Sandbox
