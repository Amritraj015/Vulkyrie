#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;

    class SandboxLayerSphere final : public Layer {
        public:
            SandboxLayerSphere()
                : app(Vulkyrie::Core::Application::GetSingleton())
                , audioSystem(CreateRef<Vulkyrie::Audio::AudioSystem>())
                , camera(Camera::Create()) {

                fahAudioClip = audioSystem->LoadClip("assets/sounds/fahhh.wav");
                akAudioClip = audioSystem->LoadClip("assets/sounds/csgo-ak.wav");

                // Load shader and texture.
                sphereShader = Shader::Create("assets/shaders/color.glsl");

                // Assert that shader and texture are loaded successfully.
                assert(sphereShader->IsValid());

                // Generate sphere vertices and indices.
                CreateSphere(1.0f, 100, 100);

                // Create the vertex array for the sphere.
                sphereVAO = VertexArray::Create();
                auto vertexBuffer = VertexBuffer::Create(sphereVertices.data(), sphereVertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPosition" },
                    { ShaderDataType::Float3, "aNormal" },
                    { ShaderDataType::Float2, "aTextureCoords" },
                });
                sphereVAO->AddVertexBuffer(vertexBuffer);
                auto indexBuffer = IndexBuffer::Create(sphereIndices.data(), sphereIndices.size());
                sphereVAO->SetIndexBuffer(indexBuffer);

                // Create the vertex array for the floor.
                floorVAO = VertexArray::Create();
                auto floorVertexBuffer = VertexBuffer::Create(floorVertices.data(), floorVertices.size() * sizeof(f32));
                floorVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPosition" },
                    { ShaderDataType::Float3, "aNormal" },
                    { ShaderDataType::Float2, "aTextureCoords" },
                });
                floorVAO->AddVertexBuffer(floorVertexBuffer);
                auto floorIndexBuffer = IndexBuffer::Create(floorIndices.data(), floorIndices.size());
                floorVAO->SetIndexBuffer(floorIndexBuffer);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerSphere() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Update audio system (frees finished sources).
                audioSystem->Update();

                // ------------------------------------------------------------------------------------
                // Render the rotating sphere.
                // Use the shader program.
                sphereShader->Use();

                // Set the projection matrix.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                sphereShader->SetMat4Uniform("projection", projection);

                // Set the view matrix.
                glm::mat4 view = camera.GetViewMatrix();
                sphereShader->SetMat4Uniform("view", view);

                // Set the model matrix (rotate over time).
                glm::mat4 model = glm::mat4(1.0f);
                sphereShader->SetMat4Uniform("model", model);

                // Set the color uniform.
                sphereShader->SetVec3Uniform("color", 1.0f, 0.3f, 0.31f);

                // Draw the sphere.
                sphereVAO->Bind();
                glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
                sphereVAO->Unbind();

                // ------------------------------------------------------------------------------------
                // Render the floor.
                // Set the model matrix for the floor.
                model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -3.0f, 0.0f));
                sphereShader->SetMat4Uniform("model", model);

                // Set the color uniform for the floor.
                sphereShader->SetVec3Uniform("color", 1.0f, 1.0f, 0.31f);

                // Draw the sphere.
                floorVAO->Bind();
                glDrawElements(GL_TRIANGLES, floorIndices.size(), GL_UNSIGNED_INT, 0);
                floorVAO->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });

                dispatcher.Dispatch<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent &e) {
                    if (e.MouseButton == MouseButton::MouseButton1) {
                        audioSystem->PlaySound(fahAudioClip);
                        return true;
                    } else if (e.MouseButton == MouseButton::MouseButton2) {
                        audioSystem->PlaySound(akAudioClip);
                        return true;
                    }

                    return false;
                });
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Sphere");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Sphere");
            }

            void CreateSphere(f32 radius, u32 stacks, u32 sectors) {
                f32 pi = std::numbers::pi;

                for (u32 i = 0; i <= stacks; ++i) {
                    f32 v = (f32)i / stacks;
                    f32 phi = v * pi;

                    for (u32 j = 0; j <= sectors; ++j) {
                        f32 u = (f32)j / sectors;
                        f32 theta = u * 2 * pi;

                        f32 x = radius * sin(phi) * cos(theta);
                        f32 y = radius * cos(phi);
                        f32 z = radius * sin(phi) * sin(theta);

                        sphereVertices.push_back(x);          // x
                        sphereVertices.push_back(y);          // y
                        sphereVertices.push_back(z);          // z
                        sphereVertices.push_back(x / radius); // nx
                        sphereVertices.push_back(y / radius); // ny
                        sphereVertices.push_back(z / radius); // nz
                        sphereVertices.push_back(u);          // u
                        sphereVertices.push_back(v);          // v
                    }
                }

                for (u32 i = 0; i < stacks; ++i) {
                    for (u32 j = 0; j < sectors; ++j) {
                        u32 first = i * (sectors + 1) + j;
                        u32 second = first + sectors + 1;

                        sphereIndices.push_back(first);
                        sphereIndices.push_back(second);
                        sphereIndices.push_back(first + 1);

                        sphereIndices.push_back(second);
                        sphereIndices.push_back(second + 1);
                        sphereIndices.push_back(first + 1);
                    }
                }
            }

        private:
            Vulkyrie::Core::Application &app;
            Camera camera;
            Ref<Vulkyrie::Audio::AudioSystem> audioSystem;
            Vulkyrie::Audio::AudioClip *fahAudioClip;
            Vulkyrie::Audio::AudioClip *akAudioClip;

            Ref<VertexArray> sphereVAO;
            Ref<Shader> sphereShader;
            std::vector<f32> sphereVertices;
            std::vector<u32> sphereIndices;

            Ref<VertexArray> floorVAO;
            std::vector<f32> floorVertices = {
                // positions         // normals        // texture coords
                5.0f,  -0.5f, 5.0f,  0.0f, 1.0f, 0.0f, 2.0f, 0.0f, //
                -5.0f, -0.5f, 5.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, //
                -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f, //
                5.0f,  -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 2.0f, 2.0f  //
            };
            std::vector<u32> floorIndices = {
                0, 1, 2, //
                0, 2, 3, //
            };
    };

} // namespace Sandbox
