#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;

    constexpr u16 WIDTH = 100;
    constexpr u16 HEIGHT = 100;
    constexpr f32 worldSizeX = 100.0f; // meters wide
    constexpr f32 worldSizeZ = 100.0f; // meters deep
    constexpr f32 heightScale = 15.0f; // max height in meters

    class SandboxLayerTerrainGeneration final : public Vulkyrie::Core::Layer {
        public:
            SandboxLayerTerrainGeneration()
                : terrainShader(Shader::Create("assets/shaders/terrain.glsl")) {

                if (!terrainShader->IsValid()) {
                    VERROR("Failed to load terrain shaders.");
                    return;
                }

                vertices.reserve(WIDTH * HEIGHT * 3);

                CreateVertexBufferElements();

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
            }

            void OnAttached() override { VDEBUG("Layer Attached: Terrain Generation"); }
            void OnDetached() override { VDEBUG("Layer Detached: Terrain Generation"); }

            void OnUpdate(const Timestep &deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                camera.OnUpdate(deltaTime);

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::M)) {
                    if (scale < 100.0f) scale += 1.0f;
                    CreateVertexBufferElements();
                }

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::N)) {
                    if (scale > 2.0f) scale -= 1.0f;
                    CreateVertexBufferElements();
                }

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::Y)) {
                    seed += 1.0f;
                    CreateVertexBufferElements();
                }

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::U)) {
                    persistence += 0.001f;
                    CreateVertexBufferElements();
                }

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::H)) {
                    offset = glm::vec2(offset.x - 0.01f, offset.y);
                    CreateVertexBufferElements();
                }

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::L)) {
                    offset = glm::vec2(offset.x, offset.y + 0.01f);
                    CreateVertexBufferElements();
                }

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::I)) {
                    lacunarity += 0.1f;
                    CreateVertexBufferElements();
                }

                if (Vulkyrie::Input::IsKeyPressed(KeyCode::O)) {
                    octaves += 1.0f;
                    CreateVertexBufferElements();
                }

                // --------------------------------------------------------------------
                // Render terrain.
                terrainShader->Use();

                // Projection Matrix.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        1000.0f);
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
                // --------------------------------------------------------------------
            }

        private:
            Camera camera;
            Ref<Shader> terrainShader;
            Ref<VertexArray> vertexArray;
            std::vector<f32> noiseMap;
            std::vector<f32> vertices;
            std::vector<u32> indices;

            f32 scale = 5.0f;
            i32 octaves = 4;
            f32 persistence = 0.5f;
            f32 lacunarity = 2.0f;
            u32 seed = 42;
            glm::vec2 offset = glm::vec2(10.0f, 10.0f);

            void CreateVertexBufferElements() {
                noiseMap = Vulkyrie::Core::GeneratePerlinNoiseMap({
                    .MapWidth = WIDTH,
                    .MapHeight = HEIGHT,
                    .Scale = scale,
                    .Octaves = octaves,
                    .Persistence = persistence,
                    .Lacunarity = lacunarity,
                    .Seed = seed,
                    .Offset = offset,
                });

                vertices.clear();

                for (size_t z = 0; z < HEIGHT; ++z) {
                    for (size_t x = 0; x < WIDTH; ++x) {
                        f32 h = noiseMap[z * WIDTH + x];

                        f32 worldX = (f32(x) / (WIDTH - 1)) * worldSizeX;
                        f32 worldZ = (f32(z) / (HEIGHT - 1)) * worldSizeZ;
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
                    for (size_t z = 0; z < HEIGHT - 1; ++z) {
                        for (size_t x = 0; x < WIDTH - 1; ++x) {
                            u32 i0 = z * WIDTH + x;
                            u32 i1 = z * WIDTH + x + 1;
                            u32 i2 = (z + 1) * WIDTH + x;
                            u32 i3 = (z + 1) * WIDTH + x + 1;

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

                    Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));
                    vertexBuffer->SetLayout({
                        { ShaderDataType::Float3, "position" },
                        { ShaderDataType::Float3, "color" },
                    });
                    vertexArray->AddVertexBuffer(vertexBuffer);

                    Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size());
                    vertexArray->SetIndexBuffer(indexBuffer);
                } else {
                    vertexArray->GetVertexBuffers()[0]->SetData(vertices.data(), vertices.size() * sizeof(f32));
                }
            }
    };
} // namespace Sandbox
