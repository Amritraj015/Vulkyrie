#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Audio;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Physics;

    class World final {
        public:
            World(std::array<glm::vec3, 8> walls, f32 sphereRadius, std::array<glm::vec3, 2> spherePositionAndSpeed)
                : walls(walls)
                , sphereRadius(sphereRadius)
                , spherePositionAndSpeed(spherePositionAndSpeed) {
            }

            void Update(Timestep dt) {
                const auto distanceRight = glm::dot((spherePositionAndSpeed[0] - walls[0]), walls[1]);
                const auto distanceLeft = glm::dot((spherePositionAndSpeed[0] - walls[2]), walls[3]);

                if (distanceRight <= sphereRadius) {
                    spherePositionAndSpeed[1] *= -1;
                } else if (distanceLeft <= sphereRadius) {
                    spherePositionAndSpeed[1] *= -1;
                }

                spherePositionAndSpeed[0] += spherePositionAndSpeed[1] * dt.GetSeconds();
            }

            const glm::vec3 GetSpherePosition() const {
                return spherePositionAndSpeed[0];
            }

        private:
            std::array<glm::vec3, 8> walls;
            f32 sphereRadius;
            std::array<glm::vec3, 2> spherePositionAndSpeed;
    };

    class SandboxLayerSphere final : public Layer {
        public:
            SandboxLayerSphere()
                : app(Application::GetSingleton())
                , audioSystem(CreateRef<AudioSystem>())
                , camera(Camera::Create())
                , sphere(1.0f, 100, 100)
                , world(
                      {
                          glm::vec3(5, 0, 0),  // Left Wall.
                          glm::vec3(-1, 0, 0), // Left Wall Normal.
                          glm::vec3(-5, 0, 0), // Right Wall.
                          glm::vec3(1, 0, 0),  // Right Wall Normal.
                          glm::vec3(0, 5, 0),  // Top Wall.
                          glm::vec3(0, -1, 0), // Top Wall Normal.
                          glm::vec3(0, -5, 0), // Bottom Wall.
                          glm::vec3(0, 1, 0),  // Bottom Wall Normal.
                      },
                      1.0f,
                      {
                          glm::vec3(0.0f, 0.0f, 0.0f), // Sphere Position.
                          glm::vec3(5.0f, 0.0f, 0.0f), // Sphere Speed.
                      }) {

                camera.SetPosition(glm::vec3(0.0f, 0.0f, 20.0f));

                fahAudioClip = audioSystem->LoadClip("assets/sounds/fahhh.wav");
                akAudioClip = audioSystem->LoadClip("assets/sounds/csgo-ak.wav");

                // Load shader and texture.
                shader = Shader::Create("assets/shaders/color.glsl");

                // Assert that shader and texture are loaded successfully.
                assert(shader->IsValid());

                // Generate sphere vertices and indices.
                auto sphereVertices = sphere.GetVertices();
                auto sphereIndices = sphere.GetIndices();

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

                // Capture Mouse on focus.
                app.CaptureMouseOnFocus(true);
            }

            ~SandboxLayerSphere() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                // Clear color and depth buffers.
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Update audio system (frees finished sources).
                audioSystem->Update();

                // ------------------------------------------------------------------------------------
                // Render the rotating sphere.
                // Use the shader program.
                shader->Use();

                // Set the projection matrix.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                shader->SetMat4Uniform("projection", projection);

                // Set the view matrix.
                glm::mat4 view = camera.GetViewMatrix();
                shader->SetMat4Uniform("view", view);

                // Update the physics world.
                world.Update(deltaTime);

                glm::mat4 model = glm::translate(glm::mat4(1.0f), world.GetSpherePosition());
                shader->SetMat4Uniform("model", model);

                // Set the color uniform.
                shader->SetVec3Uniform("color", 1.0f, 0.3f, 0.31f);

                // Draw the sphere.
                sphereVAO->Bind();
                auto &sphereIndices = sphere.GetIndices();
                glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
                sphereVAO->Unbind();

                // ------------------------------------------------------------------------------------
                // Render the floor.
                for (i32 i = 0; i < 4; ++i) {
                    // Set the model matrix for the floor.
                    switch (i) {
                        case 0:
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -4.5f, 0.0f));
                            break;
                        case 1:
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.5f, 0.0f));
                            break;
                        case 2:
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(4.5f, 0.0f, 0.0f));
                            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                            break;
                        case 3:
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.5f, 0.0f, 0.0f));
                            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                            break;
                    }

                    shader->SetMat4Uniform("model", model);

                    // Set the color uniform for the floor.
                    shader->SetVec3Uniform("color", 0.2f, 0.3f, 0.31f);

                    // Draw the sphere.
                    floorVAO->Bind();
                    glDrawElements(GL_TRIANGLES, floorIndices.size(), GL_UNSIGNED_INT, 0);
                    floorVAO->Unbind();
                }
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

        private:
            Application &app;
            Camera camera;

            Ref<AudioSystem> audioSystem;
            AudioClip *fahAudioClip;
            AudioClip *akAudioClip;

            Ref<Shader> shader;

            Sphere sphere;
            World world;
            Ref<VertexArray> sphereVAO;

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
