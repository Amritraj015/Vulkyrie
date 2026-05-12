#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <map>

namespace Sandbox {
    using namespace Vulkyrie;

    class SandboxLayerDepthAndStencilTesting final : public Layer {
        public:
            SandboxLayerDepthAndStencilTesting()
                : app(Application::GetSingleton())
                , camera(Camera::Create())
                , showDepthValues(false) {

                camera.SetMovementSpeed(1.0f, 5.0f, 20.0f);

                // Load cube and plane textures.
                cubeTexture = Texture2D::Create("assets/textures/marble.jpg");
                planeTexture = Texture2D::Create("assets/textures/metal.png");
                transparentTexture = Texture2D::Create("assets/textures/transparent_window.png");

                // Load and compile shader program.
                textureShader = Shader::Create("assets/shaders/texture_2D.glsl");
                depthTestShader = Shader::Create("assets/shaders/depth_test.glsl");

                // Assert that textures and shaders are loaded successfully.
                assert(cubeTexture->IsLoaded());
                assert(planeTexture->IsLoaded());
                assert(transparentTexture->IsLoaded());
                assert(textureShader->IsValid());
                assert(depthTestShader->IsValid());

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

                // Create vegetation vertex array.
                transparentTextureVertexArray = VertexArray::Create();
                Ref<VertexBuffer> vegetationVertexBuffer =
                    VertexBuffer::Create(transparentTextureVertices.data(), transparentTextureVertices.size() * sizeof(glm::vec3));
                vegetationVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "position" },
                    { ShaderDataType::Float2, "texture_coordinates" },
                });
                transparentTextureVertexArray->AddVertexBuffer(vegetationVertexBuffer);

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);

                // Enable blending for transparency.
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // Sort the transparent textures based on the initial distance from the camera.
                for (u32 i = 0; i < transparentTextureLocations.size(); i++) {
                    f32 distance = glm::length2(camera.GetPosition() - transparentTextureLocations[i]);
                    sortedTransparentTextures[distance] = transparentTextureLocations[i];
                }
            }

            void OnSuspended() override {
                glDisable(GL_BLEND);
            }

            void OnResumed() override {
                glEnable(GL_BLEND);
            }

            ~SandboxLayerDepthAndStencilTesting() override = default;

            void OnUpdate(Timestep deltaTime) override {
                VLKY_PROFILE_FUNCTION();

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Choose shader based on whether to show depth values.
                auto shaderToUse = showDepthValues ? depthTestShader : textureShader;

                // Use the shader program.
                shaderToUse->Use();

                // Projection transformations.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)app.GetWindowWidth() / (f32)app.GetWindowHeight(), 0.1f, 1000.0f);
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

                // Draw vegetation
                transparentTextureVertexArray->Bind();
                transparentTexture->Bind(0);
                for (std::map<f32, glm::vec3>::reverse_iterator it = sortedTransparentTextures.rbegin(); it != sortedTransparentTextures.rend(); ++it) {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), it->second);
                    shaderToUse->SetMat4Uniform("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                transparentTextureVertexArray->Unbind();
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e) {
                    camera.ProcessMouseMovement(e.MouseX, e.MouseY);

                    return true;
                });

                dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e) {
                    if (e.KeyCode == KeyCode::U) {
                        showDepthValues = !showDepthValues;

                        return true;
                    }

                    if (e.KeyCode == KeyCode::W || e.KeyCode == KeyCode::A || e.KeyCode == KeyCode::S || e.KeyCode == KeyCode::D) {
                        sortedTransparentTextures.clear();
                        for (u32 i = 0; i < transparentTextureLocations.size(); i++) {
                            f32 distance = glm::length2(camera.GetPosition() - transparentTextureLocations[i]);
                            sortedTransparentTextures[distance] = transparentTextureLocations[i];
                        }

                        return true;
                    }

                    return false;
                });
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Depth and Stencil Testing");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Depth and Stencil Testing");
            }

        private:
            Application &app;
            Camera camera;

            Ref<Texture2D> cubeTexture;
            Ref<VertexArray> cubeVertexArray;

            Ref<Texture2D> planeTexture;
            Ref<VertexArray> planeVertexArray;

            Ref<Texture2D> transparentTexture;
            Ref<VertexArray> transparentTextureVertexArray;

            Ref<Shader> textureShader;
            Ref<Shader> depthTestShader;

            bool showDepthValues;

            std::map<f32, glm::vec3> sortedTransparentTextures;

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

            std::vector<f32> transparentTextureVertices = {
                // positions       // texture Coords (swapped y coordinates because texture is flipped upside down)
                0.0f, 0.5f,  0.0f, 0.0f, 1.0f, //
                0.0f, -0.5f, 0.0f, 0.0f, 0.0f, //
                1.0f, -0.5f, 0.0f, 1.0f, 0.0f, //

                0.0f, 0.5f,  0.0f, 0.0f, 1.0f, //
                1.0f, -0.5f, 0.0f, 1.0f, 0.0f, //
                1.0f, 0.5f,  0.0f, 1.0f, 1.0f  //
            };

            std::vector<glm::vec3> transparentTextureLocations = {
                glm::vec3(-1.5f, 0.0f, -0.48f), //
                glm::vec3(1.5f, 0.0f, 0.51f),   //
                glm::vec3(0.0f, 0.0f, 0.7f),    //
                glm::vec3(-0.3f, 0.0f, -2.3f),  //
                glm::vec3(0.5f, 0.0f, -0.6f),   //
            };
    };
} // namespace Sandbox
