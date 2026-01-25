#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;

    class SandboxDepthAndStencilTesting final : public Layer {
        public:
            SandboxDepthAndStencilTesting(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application), camera(glm::vec3(0.0f, 0.0f, 5.0f)), windowWidth(windowWidth), windowHeight(windowHeight) {

                // Load cube and plane textures.
                cubeTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/marble.jpg");
                planeTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/metal.png");

                if (!cubeTexture->IsLoaded() || !planeTexture->IsLoaded()) {
                    VERROR("Failed to load one or more textures!");
                }

                // Load and compile shader program.
                shader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/texture.glsl");

                if (!shader->IsValid()) {
                    VERROR("Failed to create shader program!");
                }

                // Create cube vertex array.
                cubeVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);
                Ref<VertexBuffer> cubeVertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, cubeVertices.data(), cubeVertices.size() * sizeof(f32));
                cubeVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                cubeVertexArray->AddVertexBuffer(cubeVertexBuffer);

                // Create plane vertex array.
                planeVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);
                Ref<VertexBuffer> planeVertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, planeVertices.data(), planeVertices.size() * sizeof(f32));
                planeVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                planeVertexArray->AddVertexBuffer(planeVertexBuffer);

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
            }

            ~SandboxDepthAndStencilTesting() override = default;

            void OnUpdate(const Timestep deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                f32 cameraSpeed = 5.0f;
                auto dt = deltaTime.GetSeconds();

                if (_application.IsKeyPressed(KeyCode::LeftShift)) cameraSpeed = 20.0f;

                if (_application.IsKeyPressed(KeyCode::W)) camera.ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::S)) camera.ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::A)) camera.ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::D)) camera.ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);

                // Use the shader program.
                shader->Use();

                // Projection transformations.
                glm::mat4 projection = glm::perspective(glm::radians(45.0f), (f32)windowWidth / (f32)windowHeight, 0.1f, 100.0f);
                shader->SetMat4Uniform("projection", projection);

                // View transform
                glm::mat4 view = camera.GetViewMatrix();
                shader->SetMat4Uniform("view", view);

                // Draw cubes
                cubeTexture->Bind(0);
                cubeVertexArray->Bind();

                // First cube
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, -1.0f));
                shader->SetMat4Uniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // Second cube
                model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));
                shader->SetMat4Uniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                cubeVertexArray->Unbind();

                // Draw plane
                planeVertexArray->Bind();
                planeTexture->Bind(0);
                model = glm::mat4(1.0f);
                shader->SetMat4Uniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                planeVertexArray->Unbind();
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Depth and Stencil Testing");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Depth and Stencil Testing");
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    auto mouseMovedEvent = static_cast<MouseMovedEvent>(e);

                    if (firstMouseMove) {
                        lastMouseX = mouseMovedEvent.MouseX;
                        lastMouseY = mouseMovedEvent.MouseY;
                        firstMouseMove = false;
                    }

                    const f32 xOffset = mouseMovedEvent.MouseX - lastMouseX;
                    const f32 yOffset = lastMouseY - mouseMovedEvent.MouseY;

                    camera.ProcessMouseMovement(xOffset, yOffset);

                    lastMouseX = mouseMovedEvent.MouseX;
                    lastMouseY = mouseMovedEvent.MouseY;

                    return true;
                });
            }

        private:
            Camera camera;
            f64 lastMouseX = 400.0f;
            f64 lastMouseY = 300.0f;
            bool firstMouseMove = true;
            f32 windowHeight;
            f32 windowWidth;

            Ref<Texture2D> cubeTexture;
            Ref<VertexArray> cubeVertexArray;

            Ref<Texture2D> planeTexture;
            Ref<VertexArray> planeVertexArray;

            Ref<Shader> shader;

            std::vector<f32> cubeVertices = {
                // positions          // texture Coords
                -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f, 0.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, //

                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 1.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 1.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //

                -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, //
                -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //
                -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, //

                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, //
                0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //

                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //
                0.5f,  -0.5f, -0.5f, 1.0f, 1.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, //
                0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, //
                -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, //
                -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, //

                -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, //
                0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, //
                -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, //
                -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f  //
            };

            std::vector<f32> planeVertices = {
                // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause
                // the
                // floor texture to repeat)
                5.0f,  -0.5f, 5.0f,  2.0f, 0.0f, //
                -5.0f, -0.5f, 5.0f,  0.0f, 0.0f, //
                -5.0f, -0.5f, -5.0f, 0.0f, 2.0f, //

                5.0f,  -0.5f, 5.0f,  2.0f, 0.0f, //
                -5.0f, -0.5f, -5.0f, 0.0f, 2.0f, //
                5.0f,  -0.5f, -5.0f, 2.0f, 2.0f  //
            };
    };
} // namespace Sandbox
