#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;

    class SandboxLayerDeferredShading final : public Layer {
        public:
            SandboxLayerDeferredShading()
                : app(Application::GetSingleton())
                , camera(Camera::Create()) {

                // Load and compile shader program.
                deferredShader = Shader::Create("assets/shaders/deferred_shading.glsl");
                gBufferShader = Shader::Create("assets/shaders/g_buffer.glsl");
                shaderLightBox = Shader::Create("assets/shaders/deferred_light_box.glsl");
                backPackModel = Model::Create("assets/models/backpack/backpack.obj");

                // Assert that shader and model are loaded successfully.
                assert(deferredShader->IsValid());
                assert(gBufferShader->IsValid());
                assert(shaderLightBox->IsValid());

                // Create cube vertex array.
                cubeVertexArray = VertexArray::Create();
                Ref<VertexBuffer> cubeVertexBuffer = VertexBuffer::Create(cubeVertices.data(), cubeVertices.size() * sizeof(f32));
                cubeVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float3, "normal" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                cubeVertexArray->AddVertexBuffer(cubeVertexBuffer);

                // Create quad vertex array.
                quadVertexArray = VertexArray::Create();
                Ref<VertexBuffer> quadVertexBuffer = VertexBuffer::Create(quadVertices.data(), quadVertices.size() * sizeof(f32));
                quadVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                quadVertexArray->AddVertexBuffer(quadVertexBuffer);

                // Create G-Buffer.
                gBuffer = FrameBuffer::Create({
                    .Width = app.GetWindowWidth(),
                    .Height = app.GetWindowHeight(),
                    .ColorAttachments =
                        std::vector<ColorAttachmentSpecification>{
                            { .Format = ColorFormat::RGBA16F, .Type = AttachmentType::Texture }, // Position
                            { .Format = ColorFormat::RGBA16F, .Type = AttachmentType::Texture }, // Normal
                            { .Format = ColorFormat::RGBA8, .Type = AttachmentType::Texture },   // Albedo
                        },
                    .DepthStencilAttachment =
                        DepthStencilAttachmentSpecification{
                            .Format = DepthStencilFormat::Depth24Stencil8,
                            .Type = AttachmentType::RenderBuffer,
                        },
                });

                // lighting info
                // -------------
                const u32 NR_LIGHTS = 32;
                srand(13);
                for (unsigned int i = 0; i < NR_LIGHTS; i++) {
                    // calculate slightly random offsets
                    f32 xPos = static_cast<f32>(((rand() % 100) / 100.0) * 6.0 - 3.0);
                    f32 yPos = static_cast<f32>(((rand() % 100) / 100.0) * 6.0 - 4.0);
                    f32 zPos = static_cast<f32>(((rand() % 100) / 100.0) * 6.0 - 3.0);
                    lightPositions.push_back(glm::vec3(xPos, yPos, zPos));

                    // also calculate random color
                    f32 rColor = static_cast<f32>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
                    f32 gColor = static_cast<f32>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
                    f32 bColor = static_cast<f32>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
                    lightColors.push_back(glm::vec3(rColor, gColor, bColor));
                }

                // shader configuration
                // --------------------
                deferredShader->Use();
                deferredShader->SetIntUniform("gPosition", 0);
                deferredShader->SetIntUniform("gNormal", 1);
                deferredShader->SetIntUniform("gAlbedoSpec", 2);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerDeferredShading() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                // Clear the color and depth buffer.
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                const f32 screenWidth = static_cast<f32>(app.GetWindowWidth());
                const f32 screenHeight = static_cast<f32>(app.GetWindowHeight());
                glm::mat4 view = camera.GetViewMatrix();
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), screenWidth / screenHeight, 0.1f, 1000.0f);
                glm::mat4 model = glm::mat4(1.0f);

                // 1. geometry pass: render scene's geometry/color data into gbuffer
                // -----------------------------------------------------------------
                gBuffer->Bind();
                {
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    gBufferShader->Use();
                    gBufferShader->SetMat4Uniform("projection", projection);
                    gBufferShader->SetMat4Uniform("view", view);

                    for (u32 i = 0; i < backpackLocations.size(); i++) {
                        model = glm::mat4(1.0f);
                        model = glm::translate(model, backpackLocations[i]);
                        model = glm::scale(model, glm::vec3(0.5f));
                        gBufferShader->SetMat4Uniform("model", model);
                        backPackModel->Draw(*gBufferShader);
                    }
                }
                gBuffer->Unbind();

                // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
                // -----------------------------------------------------------------------------------------------------------------------
                {
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    deferredShader->Use();
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentResourceID(0)); // position
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentResourceID(1)); // normal
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentResourceID(2)); // albedo + specular

                    // send light relevant uniforms
                    for (u32 i = 0; i < lightPositions.size(); i++) {
                        deferredShader->SetVec3Uniform("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
                        deferredShader->SetVec3Uniform("lights[" + std::to_string(i) + "].Color", lightColors[i]);
                        // update attenuation parameters and calculate radius
                        const f32 linear = 0.7f;
                        const f32 quadratic = 1.8f;
                        deferredShader->SetFloatUniform("lights[" + std::to_string(i) + "].Linear", linear);
                        deferredShader->SetFloatUniform("lights[" + std::to_string(i) + "].Quadratic", quadratic);
                    }
                    deferredShader->SetVec3Uniform("viewPos", camera.GetPosition());

                    // finally render quad
                    quadVertexArray->Bind();
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    quadVertexArray->Unbind();
                }

                // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
                // ----------------------------------------------------------------------------------
                {
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer->GetFrameBufferID());
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
                    // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to
                    // match. the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to
                    // write to the depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's
                    // internal format).
                    glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);

                    // 3. render lights on top of scene
                    // --------------------------------
                    shaderLightBox->Use();
                    shaderLightBox->SetMat4Uniform("projection", projection);
                    shaderLightBox->SetMat4Uniform("view", view);
                    cubeVertexArray->Bind();
                    for (u32 i = 0; i < lightPositions.size(); i++) {
                        model = glm::mat4(1.0f);
                        model = glm::translate(model, lightPositions[i]);
                        model = glm::scale(model, glm::vec3(0.125f));
                        shaderLightBox->SetMat4Uniform("model", model);
                        shaderLightBox->SetVec3Uniform("lightColor", lightColors[i]);

                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                    cubeVertexArray->Unbind();
                }
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<WindowResizedEvent>([this](const WindowResizedEvent &e) {
                    gBuffer->Resize(e.Width, e.Height);
                    return false;
                });

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: DeferredShading");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: DeferredShading");
            }

        private:
            Application &app;
            Camera camera;

            Ref<FrameBuffer> gBuffer;
            Ref<Model> backPackModel;

            Ref<Shader> gBufferShader;
            Ref<Shader> deferredShader;
            Ref<Shader> shaderLightBox;

            Ref<VertexArray> quadVertexArray;
            Ref<VertexArray> cubeVertexArray;

            std::vector<glm::vec3> lightPositions;
            std::vector<glm::vec3> lightColors;

            std::vector<glm::vec3> backpackLocations = {
                glm::vec3(-3.0, -0.5, -3.0), //
                glm::vec3(0.0, -0.5, -3.0),  //
                glm::vec3(3.0, -0.5, -3.0),  //
                glm::vec3(-3.0, -0.5, 0.0),  //
                glm::vec3(0.0, -0.5, 0.0),   //
                glm::vec3(3.0, -0.5, 0.0),   //
                glm::vec3(-3.0, -0.5, 3.0),  //
                glm::vec3(0.0, -0.5, 3.0),   //
                glm::vec3(3.0, -0.5, 3.0),   //
            };

            std::vector<f32> quadVertices = {
                // positions        // texture Coords
                -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, //
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, //
                1.0f,  1.0f,  0.0f, 1.0f, 1.0f, //
                1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, //
            };

            std::vector<f32> cubeVertices = {
                // back face
                -1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f, // top-right
                1.0f,  -1.0f, -1.0f, 0.0f,  0.0f,  -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f, 1.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f, 1.0f, // top-left

                -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // bottom-left
                1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // top-right
                -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // top-left
                -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // bottom-left

                -1.0f, 1.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, // top-right
                -1.0f, 1.0f,  -1.0f, -1.0f, 0.0f,  0.0f,  1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, 1.0f,  -1.0f, 0.0f,  0.0f,  0.0f, 0.0f, // bottom-right
                -1.0f, 1.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, // top-right

                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // top-left
                1.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // top-right
                1.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // top-left
                1.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // bottom-left

                -1.0f, -1.0f, -1.0f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, // top-right
                1.0f,  -1.0f, -1.0f, 0.0f,  -1.0f, 0.0f,  1.0f, 1.0f, // top-left
                1.0f,  -1.0f, 1.0f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f, // bottom-left
                1.0f,  -1.0f, 1.0f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f, 1.0f,  0.0f,  -1.0f, 0.0f,  0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, // top-right

                -1.0f, 1.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // top-left
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // bottom-right
                -1.0f, 1.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // top-left
                -1.0f, 1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f  // bottom-left
            };
    };
} // namespace Sandbox
