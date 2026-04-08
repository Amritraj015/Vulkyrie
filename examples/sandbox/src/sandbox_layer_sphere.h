#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie;

    class World final {
        public:
            World() = default;

            World(const std::vector<Plane> &planes, std::array<std::tuple<glm::vec3, glm::vec3, f32>, 1> spheres)
                : walls(planes)
                , spheres(spheres) {
            }

            void RotateWalls(const glm::mat3 &rotationMatrix) {
                for (auto &wall : walls) {
                    wall.Rotate(rotationMatrix);
                }
            }

            void Update(Timestep dt) {
                for (auto &[spherePosition, sphereSpeed, sphereRadius] : spheres) {
                    for (const auto &wall : walls) {
                        // Calculate the signed distance from the sphere's center to the wall.
                        f32 distance = wall.GetSignedDistance(spherePosition);

                        // If the distance is less than the sphere's radius, it means the sphere is colliding with the wall.
                        if (distance < sphereRadius) {
                            const glm::vec3 &wallNormal = wall.GetNormal();

                            // Reflect the sphere's speed vector across the wall's normal to simulate a bounce.
                            sphereSpeed = glm::reflect(sphereSpeed, wallNormal);

                            // Move the sphere out of the wall to prevent it from getting stuck.
                            spherePosition += wallNormal * (sphereRadius - distance);
                        }
                    }

                    // Update the sphere's position based on its speed and the time step.
                    spherePosition += sphereSpeed * dt.GetSeconds();
                }
            }

            const glm::vec3 GetSpherePosition() const {
                return std::get<0>(spheres[0]);
            }

        private:
            /** @brief A vector of walls. */
            std::vector<Plane> walls;

            /** @brief Spheres represented by its speed, position and radius. */
            std::array<std::tuple<glm::vec3, glm::vec3, f32>, 1> spheres;
    };

    class SandboxLayerSphere final : public Layer {
        public:
            SandboxLayerSphere()
                : app(Application::GetSingleton())
                , physicsWorld(PhysicsWorldSettings("Sphere Physics World"))
                , transformComponentStore()
                , audioSystem(CreateRef<AudioSystem>())
                , camera(Camera::Create())
                , sphere(1.0f, 100, 100) {

                // Define the walls of the room as planes with their normal and distance from the origin along the normal vector.
                const std::vector<Plane> walls = {
                    Plane(glm::vec3(-1, 0, 0), -5.0f), // Left Wall.
                    Plane(glm::vec3(1, 0, 0), -5.0f),  // Right Wall.
                    Plane(glm::vec3(0, -1, 0), -5.0f), // Top Wall.
                    Plane(glm::vec3(0, 1, 0), -5.0f),  // Bottom Wall.
                };

                // Define a single sphere with its initial position, speed and radius.
                const std::array<std::tuple<glm::vec3, glm::vec3, f32>, 1> spheres = {
                    std::make_tuple<glm::vec3, glm::vec3, f32>(glm::vec3(0.0f), glm::vec3(10.0f, 15.0f, 0.0f), 1.0f), // Sphere.
                };

                // Create the physics world with the defined walls and spheres.
                world = World(walls, spheres);

                // Set initial camera position.
                camera.SetPosition(glm::vec3(0.0f, 0.0f, 20.0f));

                // Load audio clips.
                fahAudioClip = audioSystem->LoadClip("assets/sounds/fahhh.wav");
                akAudioClip = audioSystem->LoadClip("assets/sounds/csgo-ak.wav");
                deniedClip = audioSystem->LoadClip("assets/sounds/denied.wav");
                humiliationClip = audioSystem->LoadClip("assets/sounds/humiliation.wav");

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
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -5.0f, 0.0f));
                            model = glm::scale(model, glm::vec3(5.0f));
                            break;
                        case 1:
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f));
                            model = glm::scale(model, glm::vec3(5.0f));
                            break;
                        case 2:
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
                            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                            model = glm::scale(model, glm::vec3(5.0f));
                            break;
                        case 3:
                            model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 0.0f, 0.0f));
                            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                            model = glm::scale(model, glm::vec3(5.0f));
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
                    } else if (e.MouseButton == MouseButton::MouseButton3) {
                        audioSystem->PlaySound(deniedClip);
                        return true;
                    } else if (e.MouseButton == MouseButton::MouseButton4) {
                        audioSystem->PlaySound(humiliationClip);
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

            PhysicsWorld physicsWorld;
            TransformComponentStore transformComponentStore;

            Ref<AudioSystem> audioSystem;
            AudioClip *fahAudioClip;
            AudioClip *akAudioClip;
            AudioClip *deniedClip;
            AudioClip *humiliationClip;

            Ref<Shader> shader;

            Sphere sphere;
            World world;
            Ref<VertexArray> sphereVAO;

            Ref<VertexArray> floorVAO;
            std::vector<f32> floorVertices = {
                // positions        // normals
                1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f, // Top right
                -1.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, // Top left
                -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // Bottom left
                1.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // Bottom right
            };
            std::vector<u32> floorIndices = {
                0, 1, 2, // First triangle
                0, 2, 3, // Second triangle
            };
    };

} // namespace Sandbox
