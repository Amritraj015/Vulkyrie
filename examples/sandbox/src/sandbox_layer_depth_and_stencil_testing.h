#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;

    class SandboxLayerDepthAndStencilTesting final : public Layer {
        public:
            SandboxLayerDepthAndStencilTesting(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application)
                , camera(glm::vec3(0.0f, 0.0f, 5.0f))
                , windowWidth(windowWidth)
                , windowHeight(windowHeight)
                , showDepthValues(false) {

                camera.SetMovementSpeed(5.0f, 20.0f);

                // Load cube and plane textures.
                cubeTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/marble.jpg");
                planeTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/metal.png");
                transparentTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/transparent_window.png");

                if (!cubeTexture->IsLoaded() || !planeTexture->IsLoaded() || !transparentTexture->IsLoaded()) {
                    VERROR("Failed to load one or more textures!");
                }

                // Load and compile shader program.
                textureShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/texture_2D.glsl");
                depthTestShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/depth_test.glsl");

                if (!textureShader->IsValid() || !depthTestShader->IsValid()) {
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

                // Create vegetation vertex array.
                transparentTextureVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);
                Ref<VertexBuffer> vegetationVertexBuffer =
                    VertexBuffer::Create(GraphicsAPI::OpenGL, transparentTextureVertices.data(), transparentTextureVertices.size() * sizeof(glm::vec3));
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

            ~SandboxLayerDepthAndStencilTesting() override = default;

            void OnUpdate(const Timestep &deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                // Choose shader based on whether to show depth values.
                auto shaderToUse = showDepthValues ? depthTestShader : textureShader;

                // Use the shader program.
                shaderToUse->Use();

                // Projection transformations.
                glm::mat4 projection = glm::perspective(glm::radians(45.0f), (f32)windowWidth / (f32)windowHeight, 0.1f, 100.0f);
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

            void OnAttached() override {
                VDEBUG("Layer Attached: Depth and Stencil Testing");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Depth and Stencil Testing");
            }

            void OnEvent(Event &event) override {
                EventDispatcher dispatcher(event);

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

        private:
            Camera camera;
            f32 windowHeight;
            f32 windowWidth;

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
