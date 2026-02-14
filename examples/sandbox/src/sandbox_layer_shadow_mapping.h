#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;

    class SandboxLayerShadowMapping final : public Layer {
        public:
            SandboxLayerShadowMapping()
                : app(Application::GetSingleton())
                , camera(Camera::Create())
                , lightPosition(-2.0f, 4.0f, -1.0f) {

                camera.SetMovementSpeed(1.0f, 5.0f, 20.0f);

                // Load shaders, textures and frame buffers.
                depthBufferShader = Shader::Create("assets/shaders/default_shadow_map_depth_buffer.glsl");
                debugDepthBufferShader = Shader::Create("assets/shaders/debug_depth_quad.glsl");
                shadowShader = Shader::Create("assets/shaders/default_shadow_map.glsl");
                woodTexture = Texture2D::Create("assets/textures/wood.png");

                // Create framebuffer for shadow mapping.
                frameBuffer = FrameBuffer::Create({
                    .Width = 1024,
                    .Height = 1024,
                    .DepthStencilAttachment =
                        DepthStencilAttachmentSpecification{
                            .Format = DepthStencilFormat::Depth24Stencil8,
                            .Type = AttachmentType::Texture,
                            .MinFilter = TextureFilterMode::Nearest,
                            .MagFilter = TextureFilterMode::Nearest,
                            .WrapS = TextureSamplerWrapMode::Repeat,
                            .WrapT = TextureSamplerWrapMode::Repeat,
                            .Samples = 1,
                        },
                    .DebugName = "ShadowMapFrameBuffer",
                });

                // Assert that shader and texture are loaded successfully.
                assert(debugDepthBufferShader->IsValid());
                assert(depthBufferShader->IsValid());
                assert(shadowShader->IsValid());
                assert(woodTexture->IsLoaded());
                assert(frameBuffer->IsComplete());

                // Create vertex array for the surface.
                surfaceVertexArray = VertexArray::Create();
                Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(surfaceVertices.data(), surfaceVertices.size() * sizeof(f32));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float3, "normal" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                surfaceVertexArray->AddVertexBuffer(vertexBuffer);

                cubeVertexArray = VertexArray::Create();
                Ref<VertexBuffer> cubeVertexBuffer = VertexBuffer::Create(cubeVertices.data(), cubeVertices.size() * sizeof(f32));
                cubeVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float3, "normal" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                cubeVertexArray->AddVertexBuffer(cubeVertexBuffer);

                quadVertexArray = VertexArray::Create();
                Ref<VertexBuffer> quadVertexBuffer = VertexBuffer::Create(quadVertices.data(), quadVertices.size() * sizeof(f32));
                quadVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                quadVertexArray->AddVertexBuffer(quadVertexBuffer);

                shadowShader->Use();
                shadowShader->SetIntUniform("diffuseTexture", 0);
                shadowShader->SetIntUniform("shadowMap", 1);

                debugDepthBufferShader->Use();
                debugDepthBufferShader->SetIntUniform("depthMap", 0);

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
            }

            ~SandboxLayerShadowMapping() = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // 1. render depth of scene to texture (from light's perspective)
                // --------------------------------------------------------------
                glm::vec3 lightPos = glm::vec3(lightPosition.x * sin(app.GetTime()), lightPosition.y, lightPosition.z * cos(app.GetTime()));
                glm::mat4 lightProjection, lightView;
                glm::mat4 lightSpaceMatrix;
                f32 near_plane = 1.0f, far_plane = 7.5f;
                lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
                lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
                lightSpaceMatrix = lightProjection * lightView;

                // render scene from light's point of view
                depthBufferShader->Use();
                depthBufferShader->SetMat4Uniform("lightSpaceMatrix", lightSpaceMatrix);

                frameBuffer->Bind();
                RenderScene(depthBufferShader);
                frameBuffer->Unbind();

                {
                    // Render depth map to quad for visual debugging.
                    glViewport(0, 0, app.GetWindowWidth(), app.GetWindowHeight());
                    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // 2. render scene as normal using the generated depth/shadow map
                    // --------------------------------------------------------------
                    shadowShader->Use();
                    glm::mat4 projection =
                        glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
                    glm::mat4 view = camera.GetViewMatrix();
                    shadowShader->SetMat4Uniform("projection", projection);
                    shadowShader->SetMat4Uniform("view", view);

                    // set light uniforms
                    shadowShader->SetVec3Uniform("viewPos", camera.GetPosition());
                    shadowShader->SetVec3Uniform("lightPos", lightPos);
                    shadowShader->SetMat4Uniform("lightSpaceMatrix", lightSpaceMatrix);

                    woodTexture->Bind(0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, frameBuffer->GetDepthStencilAttachmentResourceID());

                    RenderScene(shadowShader);
                }

                // debugDepthBufferShader->Use();
                // debugDepthBufferShader->SetFloatUniform("near_plane", near_plane);
                // debugDepthBufferShader->SetFloatUniform("far_plane", far_plane);
                // glActiveTexture(GL_TEXTURE0);
                // glBindTexture(GL_TEXTURE_2D, frameBuffer->GetDepthStencilAttachmentResourceID());
                // quadVertexArray->Bind();
                // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                // quadVertexArray->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
                    if (e.KeyCode == KeyCode::I) {
                        lightPosition.y += 0.5f;
                        return true;
                    }

                    if (e.KeyCode == KeyCode::O) {
                        lightPosition.y -= 0.5f;
                        return true;
                    }

                    if (e.KeyCode == KeyCode::R) {
                        debugDepthBufferShader->Reload();
                        return true;
                    }

                    return false;
                });
            }

            void RenderScene(const Ref<Shader> &shader) {
                // floor
                glm::mat4 model = glm::mat4(1.0f);
                shader->SetMat4Uniform("model", model);
                surfaceVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 6);
                surfaceVertexArray->Unbind();

                // cubes
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
                model = glm::scale(model, glm::vec3(0.5f));
                shader->SetMat4Uniform("model", model);
                cubeVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                cubeVertexArray->Unbind();

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
                model = glm::scale(model, glm::vec3(0.5f));
                shader->SetMat4Uniform("model", model);
                cubeVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                cubeVertexArray->Unbind();

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
                model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
                model = glm::scale(model, glm::vec3(0.25));
                shader->SetMat4Uniform("model", model);
                cubeVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                cubeVertexArray->Unbind();
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Shadow Mapping");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Shadow Mapping");
            }

        private:
            Application &app;
            Camera camera;
            Ref<FrameBuffer> frameBuffer;

            Ref<Texture2D> woodTexture;

            Ref<VertexArray> surfaceVertexArray;
            Ref<VertexArray> cubeVertexArray;
            Ref<VertexArray> quadVertexArray;

            Ref<Shader> depthBufferShader;
            Ref<Shader> debugDepthBufferShader;
            Ref<Shader> shadowShader;

            glm::vec3 lightPosition;

            std::vector<f32> surfaceVertices = {
                // positions           // normals        // texcoords
                25.0f,  -0.5f, 25.0f,  0.0f, 1.0f, 0.0f, 25.0f, 0.0f,  //
                -25.0f, -0.5f, 25.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f,  //
                -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f,  25.0f, //

                25.0f,  -0.5f, 25.0f,  0.0f, 1.0f, 0.0f, 25.0f, 0.0f,  //
                -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f,  25.0f, //
                25.0f,  -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 25.0f, //
            };

            std::vector<f32> quadVertices = {
                // positions        // texture Coords
                -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, //
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, //
                1.0f,  1.0f,  0.0f, 1.0f, 1.0f, //
                1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, //
            };

            std::vector<f32> cubeVertices = {
                // positions         // normals           // texture coords
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
                -1.0f, 1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // bottom-left
            };
    };
} // namespace Sandbox
