#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;

    class SandboxLayerFrameBuffer final : public Layer {
        public:
            SandboxLayerFrameBuffer()
                : camera(Camera::Create())
                , showDepthValues(false) {

                camera.SetMovementSpeed(1.0f, 5.0f, 20.0f);

                frameBuffer = FrameBuffer::Create({
                    .Width = Application::GetSingleton().GetWindowWidth(),
                    .Height = Application::GetSingleton().GetWindowHeight(),
                    .ColorAttachments =
                        std::vector<ColorAttachmentSpecification>{
                            {
                                .Format = ColorFormat::RGBA8,
                                .Type = AttachmentType::Texture,
                            },
                        },
                    .DepthStencilAttachment =
                        DepthStencilAttachmentSpecification{
                            .Format = DepthStencilFormat::Depth24Stencil8,
                            .Type = AttachmentType::RenderBuffer,
                            .Samples = 1,
                        },
                    .SwapchainTarget = false,
                    .DebugName = "PostProcessingFrameBuffer",
                });

                // Load cube and plane textures.
                cubeTexture = Texture2D::Create("assets/textures/container.jpg");
                planeTexture = Texture2D::Create("assets/textures/metal.png");

                if (!cubeTexture->IsLoaded() || !planeTexture->IsLoaded()) {
                    VERROR("Failed to load one or more textures!");
                }

                // Load and compile shader program.
                textureShader = Shader::Create("assets/shaders/texture_2D.glsl");
                depthTestShader = Shader::Create("assets/shaders/depth_test.glsl");
                quadShader = Shader::Create("assets/shaders/frame_buffer.glsl");

                if (!textureShader->IsValid() || !depthTestShader->IsValid() || !quadShader->IsValid()) {
                    VERROR("Failed to create shader program!");
                }

                // Create cube vertex array.
                cubeVertexArray = VertexArray::Create();
                Ref<VertexBuffer> cubeVertexBuffer = VertexBuffer::Create(cubeVertices.data(), cubeVertices.size() * sizeof(f32));
                cubeVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                cubeVertexArray->AddVertexBuffer(cubeVertexBuffer);

                // Create plane vertex array.
                planeVertexArray = VertexArray::Create();
                Ref<VertexBuffer> planeVertexBuffer = VertexBuffer::Create(planeVertices.data(), planeVertices.size() * sizeof(f32));
                planeVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                planeVertexArray->AddVertexBuffer(planeVertexBuffer);

                // Create quad vertex array.
                quadVertexArray = VertexArray::Create();
                Ref<VertexBuffer> quadVertexBuffer = VertexBuffer::Create(quadVertices.data(), quadVertices.size() * sizeof(f32));
                quadVertexBuffer->SetLayout({
                    { ShaderDataType::Float2, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                quadVertexArray->AddVertexBuffer(quadVertexBuffer);

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerFrameBuffer() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                frameBuffer->Bind();
                glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

                // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Choose shader based on whether to show depth values.
                auto shaderToUse = showDepthValues ? depthTestShader : textureShader;

                // Use the shader program.
                shaderToUse->Use();

                // Projection transformations.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        1000.0f);
                shaderToUse->SetMat4Uniform("projection", projection);

                // View transform
                glm::mat4 view = camera.GetViewMatrix();
                shaderToUse->SetMat4Uniform("view", view);

                // Draw cubes
                cubeTexture->Bind(0);
                cubeVertexArray->Bind();

                // First cube
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, -1.0f));
                shaderToUse->SetMat4Uniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // Second cube
                model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));
                shaderToUse->SetMat4Uniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                cubeVertexArray->Unbind();

                // Draw plane
                planeVertexArray->Bind();
                planeTexture->Bind(0);
                model = glm::mat4(1.0f);
                shaderToUse->SetMat4Uniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                planeVertexArray->Unbind();

                // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
                frameBuffer->Unbind();

                glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
                // clear all relevant buffers
                // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                quadShader->Use();
                quadShader->SetIntUniform("screenTexture", 0);
                quadVertexArray->Bind();
                glBindTexture(GL_TEXTURE_2D, frameBuffer->GetColorAttachmentResourceID(0)); // use the color attachment texture as the texture of the quad plane
                glDrawArrays(GL_TRIANGLES, 0, 6);
                quadVertexArray->Unbind();

                // NOTE: This is needed for other layers, but this can be done with layer events.
                glEnable(GL_DEPTH_TEST);
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Frame buffer");
            }
            void OnDetached() override {
                VDEBUG("Layer Detached: Frame buffer");
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<WindowResizedEvent>([this](const WindowResizedEvent &e) {
                    frameBuffer->Resize(e.Width, e.Height);

                    return false;
                });

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
                    if (e.KeyCode == KeyCode::U) {
                        showDepthValues = !showDepthValues;

                        return true;
                    }

                    return false;
                });
            }

        private:
            Camera camera;
            Ref<FrameBuffer> frameBuffer;

            Ref<Texture2D> cubeTexture;
            Ref<VertexArray> cubeVertexArray;

            Ref<Texture2D> planeTexture;
            Ref<VertexArray> planeVertexArray;

            Ref<VertexArray> quadVertexArray;

            Ref<Shader> textureShader;
            Ref<Shader> depthTestShader;
            Ref<Shader> quadShader;

            bool showDepthValues;

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

            std::vector<f32> quadVertices = {
                // positions  // texCoords
                -1.0f, 1.0f,  0.0f, 1.0f, //
                -1.0f, -1.0f, 0.0f, 0.0f, //
                1.0f,  -1.0f, 1.0f, 0.0f, //

                -1.0f, 1.0f,  0.0f, 1.0f, //
                1.0f,  -1.0f, 1.0f, 0.0f, //
                1.0f,  1.0f,  1.0f, 1.0f  //
            };
    };
} // namespace Sandbox
