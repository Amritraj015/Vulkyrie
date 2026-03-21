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
            World() = default;

            World(std::array<std::pair<glm::vec3, f32>, 4> planes, std::array<std::tuple<glm::vec3, glm::vec3, f32>, 1> spheres)
                : spheres(spheres) {

                const auto &[rightWallNormal, rightWallDistance] = planes[0];
                const glm::vec3 rightWallPoint = glm::vec3(rightWallNormal * rightWallDistance * -1.0f);

                const auto &[leftWallNormal, leftWallDistance] = planes[1];
                const glm::vec3 leftWallPoint = glm::vec3(leftWallNormal * leftWallDistance * -1.0f);

                const auto &[topWallNormal, topWallDistance] = planes[2];
                const glm::vec3 topWallPoint = glm::vec3(topWallNormal * topWallDistance * -1.0f);

                const auto &[bottomWallNormal, bottomWallDistance] = planes[3];
                const glm::vec3 bottomWallPoint = glm::vec3(bottomWallNormal * bottomWallDistance * -1.0f);

                walls = {
                    std::make_pair(rightWallNormal, rightWallPoint),
                    std::make_pair(leftWallNormal, leftWallPoint),
                    std::make_pair(topWallNormal, topWallPoint),
                    std::make_pair(bottomWallNormal, bottomWallPoint),
                };
            }

            void Update(Timestep dt) {
                for (auto &[spherePosition, sphereSpeed, sphereRadius] : spheres) {
                    const f32 distanceRight = glm::dot((spherePosition - walls[0].second), walls[0].first);
                    const f32 distanceLeft = glm::dot((spherePosition - walls[1].second), walls[1].first);
                    const f32 distanceTop = glm::dot((spherePosition - walls[2].second), walls[2].first);
                    const f32 distanceBottom = glm::dot((spherePosition - walls[3].second), walls[3].first);

                    if (distanceRight <= sphereRadius && sphereSpeed.x > 0.0f) {
                        sphereSpeed *= glm::vec3(-1.0f, 1.0f, 1.0f);
                    } else if (distanceLeft <= sphereRadius && sphereSpeed.x < 0.0f) {
                        sphereSpeed *= glm::vec3(-1.0f, 1.0f, 1.0f);
                    } else if (distanceTop <= sphereRadius && sphereSpeed.y > 0.0f) {
                        sphereSpeed *= glm::vec3(1.0f, -1.0f, 1.0f);
                    } else if (distanceBottom <= sphereRadius && sphereSpeed.y < 0.0f) {
                        sphereSpeed *= glm::vec3(1.0f, -1.0f, 1.0f);
                    }

                    spherePosition += sphereSpeed * dt.GetSeconds();
                }
            }

            const glm::vec3 GetSpherePosition() const {
                return std::get<0>(spheres[0]);
            }
            //
            // enum Wall { Left, Right, Top, Bottom };
            //
            // const std::tuple<glm::mat4, glm::vec3, glm::vec2> GetRightWall() const {
            // }
            //
            // const std::tuple<glm::mat4, glm::vec3, glm::vec2> GetLeftWall() const {
            // }
            //
            // const std::tuple<glm::mat4, glm::vec3, glm::vec2> GetTopWall() const {
            // }
            //
            // const std::tuple<glm::mat4, glm::vec3, glm::vec2> GetBottomWall() const {
            // }

        private:
            /** @brief 4 walls of a room represented by their normal and distance from the origin. */
            std::array<std::pair<glm::vec3, glm::vec3>, 4> walls;

            /** @brief Spheres represented by its speed, position and radius. */
            std::array<std::tuple<glm::vec3, glm::vec3, f32>, 1> spheres;
    };

    class SandboxLayerSphere final : public Layer {
        public:
            SandboxLayerSphere()
                : app(Application::GetSingleton())
                , audioSystem(CreateRef<AudioSystem>())
                , camera(Camera::Create())
                , sphere(1.0f, 100, 100) {

                // Define the walls of the room as planes with their normal and distance from the origin.
                const std::array<std::pair<glm::vec3, f32>, 4> walls = {
                    std::make_pair<glm::vec3, f32>(glm::vec3(-1, 0, 0), 5.0f), // Left Wall.
                    std::make_pair<glm::vec3, f32>(glm::vec3(1, 0, 0), 5.0f),  // Right Wall.
                    std::make_pair<glm::vec3, f32>(glm::vec3(0, -1, 0), 5.0f), // Top Wall.
                    std::make_pair<glm::vec3, f32>(glm::vec3(0, 1, 0), 5.0f),  // Bottom Wall.
                };

                // Define a single sphere with its initial position, speed and radius.
                const std::array<std::tuple<glm::vec3, glm::vec3, f32>, 1> spheres = {
                    std::make_tuple<glm::vec3, glm::vec3, f32>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 15.0f, 0.0f), 1.0f), // Sphere.
                };

                // Create the physics world with the defined walls and spheres.
                world = World(walls, spheres);

                // Set initial camera position.
                camera.SetPosition(glm::vec3(0.0f, 0.0f, 20.0f));

                // Load audio clips.
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
                1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //
                -1.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, //
                -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, //
                1.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f  //
            };
            std::vector<u32> floorIndices = {
                0, 1, 2, //
                0, 2, 3, //
            };
    };

} // namespace Sandbox
