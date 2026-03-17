#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Audio;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Physics;

    class SandboxLayerSphere final : public Layer {
        public:
            SandboxLayerSphere()
                : app(Application::GetSingleton())
                , audioSystem(CreateRef<AudioSystem>())
                , camera(Camera::Create())
                , sphere(1.0f, 100, 100) {

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

                // Set the model matrix (rotate over time).
                const f32 time = deltaTime.GetSeconds();
                spherePosition += sphereSpeed * time;

                DetectCollisions();

                glm::mat4 model = glm::translate(glm::mat4(1.0f), spherePosition);
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

            void DetectCollisions() {
                glm::vec3 floorNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                glm::vec3 floorPosition = glm::vec3(0.0f, -4.5f, 0.0f);

                f32 distance = glm::dot(spherePosition - floorPosition, floorNormal);
                if (distance < 1.0f) {
                    // Collision detected, reflect the sphere's velocity.
                    sphereSpeed = glm::reflect(sphereSpeed, floorNormal);
                    spherePosition += floorNormal * (1.0f - distance); // Move the sphere out of the floor.
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

            glm::vec3 sphereSpeed = glm::vec3(0.0f, -5.0f, 0.0f);
            glm::vec3 spherePosition = glm::vec3(0.0f, 0.0f, 0.0f);

            Ref<Shader> shader;

            Sphere sphere;
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
