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
                , camera(Camera::Create()) {

                // Load shader and texture.
                shader = Shader::Create("assets/shaders/model.glsl");
                texture = Texture2D::Create("assets/textures/container.jpg");

                // Assert that shader and texture are loaded successfully.
                assert(shader->IsValid());
                assert(texture->IsLoaded());

                // Generate sphere vertices and indices.
                CreateSphere(1.0f, 100, 100);

                // Create the vertex array for the sphere.
                sphereVAO = VertexArray::Create();

                // Create vertex buffer and set layout.
                auto vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float3, "normal" },
                    { ShaderDataType::Float2, "texCoords" },
                });
                sphereVAO->AddVertexBuffer(vertexBuffer);

                // Create index buffer.
                auto indexBuffer = IndexBuffer::Create(indices.data(), indices.size());
                sphereVAO->SetIndexBuffer(indexBuffer);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Use the shader program.
                shader->Use();

                // Bind texture.
                texture->Bind(0);

                // Set the projection matrix.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                shader->SetMat4Uniform("projection", projection);

                // Set the view matrix.
                glm::mat4 view = camera.GetViewMatrix();
                shader->SetMat4Uniform("view", view);

                // Set the model matrix (rotate over time).
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::rotate(model, (f32)app.GetTime() * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                shader->SetMat4Uniform("model", model);

                // Draw the sphere.
                sphereVAO->Bind();
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
                sphereVAO->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
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

                        vertices.push_back(x);          // x
                        vertices.push_back(y);          // y
                        vertices.push_back(z);          // z
                        vertices.push_back(x / radius); // nx
                        vertices.push_back(y / radius); // ny
                        vertices.push_back(z / radius); // nz
                        vertices.push_back(u);          // u
                        vertices.push_back(v);          // v
                    }
                }

                for (u32 i = 0; i < stacks; ++i) {
                    for (u32 j = 0; j < sectors; ++j) {
                        u32 first = i * (sectors + 1) + j;
                        u32 second = first + sectors + 1;

                        indices.push_back(first);
                        indices.push_back(second);
                        indices.push_back(first + 1);

                        indices.push_back(second);
                        indices.push_back(second + 1);
                        indices.push_back(first + 1);
                    }
                }
            }

        private:
            Vulkyrie::Core::Application &app;
            Camera camera;
            Ref<VertexArray> sphereVAO;
            Ref<Shader> shader;
            Ref<Texture2D> texture;
            std::vector<f32> vertices;
            std::vector<u32> indices;
    };

} // namespace Sandbox
