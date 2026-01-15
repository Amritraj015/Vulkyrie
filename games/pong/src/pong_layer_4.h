#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"

#include "vendor/stb_image.h"

namespace Pong {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Events;

    constexpr float vertices[] = {
        // positions        // texture coords
        0.5f,  0.5f,  0.0f, // 1.0f, 1.0f, // top right
        0.5f,  -0.5f, 0.0f, // 1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f, // 0.0f, 0.0f, // bottom left
        -0.5f, 0.5f,  0.0f, // 0.0f, 1.0f, // top left
    };

    constexpr u32 indices[] = {
        0, 1, 2, // first triangle
        0, 2, 3  // second triangle
    };

    constexpr float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f,  -1.0f, //
        -1.0f, -1.0f, -1.0f, //
        1.0f,  -1.0f, -1.0f, //
        1.0f,  -1.0f, -1.0f, //
        1.0f,  1.0f,  -1.0f, //
        -1.0f, 1.0f,  -1.0f, //

        -1.0f, -1.0f, 1.0f,  //
        -1.0f, -1.0f, -1.0f, //
        -1.0f, 1.0f,  -1.0f, //
        -1.0f, 1.0f,  -1.0f, //
        -1.0f, 1.0f,  1.0f,  //
        -1.0f, -1.0f, 1.0f,  //

        1.0f,  -1.0f, -1.0f, //
        1.0f,  -1.0f, 1.0f,  //
        1.0f,  1.0f,  1.0f,  //
        1.0f,  1.0f,  1.0f,  //
        1.0f,  1.0f,  -1.0f, //
        1.0f,  -1.0f, -1.0f, //

        -1.0f, -1.0f, 1.0f, //
        -1.0f, 1.0f,  1.0f, //
        1.0f,  1.0f,  1.0f, //
        1.0f,  1.0f,  1.0f, //
        1.0f,  -1.0f, 1.0f, //
        -1.0f, -1.0f, 1.0f, //

        -1.0f, 1.0f,  -1.0f, //
        1.0f,  1.0f,  -1.0f, //
        1.0f,  1.0f,  1.0f,  //
        1.0f,  1.0f,  1.0f,  //
        -1.0f, 1.0f,  1.0f,  //
        -1.0f, 1.0f,  -1.0f, //

        -1.0f, -1.0f, -1.0f, //
        -1.0f, -1.0f, 1.0f,  //
        1.0f,  -1.0f, -1.0f, //
        1.0f,  -1.0f, -1.0f, //
        -1.0f, -1.0f, 1.0f,  //
        1.0f,  -1.0f, 1.0f,  //
    };

    const u16 WIDTH = 100;
    const u16 HEIGHT = 100;

    class PongLayer4 final : public Vulkyrie::Core::Layer {
        public:
            PongLayer4(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application), windowWidth(windowWidth), windowHeight(windowHeight), camera(glm::vec3(0.0f, 0.0f, 5.0f)),
                  // texture(Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/wall.jpg")),
                  terrainShader(Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/terrain.glsl")) {

                // if (!texture->IsLoaded()) {
                //     VERROR("PongLayer4: Failed to load texture.");
                //     return;
                // }

                if (!terrainShader->IsValid()) {
                    VERROR("PongLayer4: Failed to load shaders.");
                    return;
                }

                vertexArray = VertexArray::Create(GraphicsAPI::OpenGL);

                Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, const_cast<float *>(vertices), sizeof(vertices));
                vertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                    // { ShaderDataType::Float2, "aTexCoord" },
                });
                vertexArray->AddVertexBuffer(vertexBuffer);

                Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(GraphicsAPI::OpenGL, const_cast<u32 *>(indices), sizeof(indices) / sizeof(u32));
                vertexArray->SetIndexBuffer(indexBuffer);

                // Load sky-box cubemap textures.
                stbi_set_flip_vertically_on_load(false);
                std::vector<std::string> faces = {
                    "assets/cubemaps/skybox/right.jpg",  "assets/cubemaps/skybox/left.jpg",  "assets/cubemaps/skybox/top.jpg",
                    "assets/cubemaps/skybox/bottom.jpg", "assets/cubemaps/skybox/front.jpg", "assets/cubemaps/skybox/back.jpg",
                };
                skyboxTextureId = loadCubemap(faces);

                skyboxVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);
                Ref<VertexBuffer> skyboxVertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, const_cast<float *>(skyboxVertices), sizeof(skyboxVertices));
                skyboxVertexBuffer->SetLayout({
                    { ShaderDataType::Float3, "aPos" },
                });
                skyboxVertexArray->AddVertexBuffer(skyboxVertexBuffer);

                // Load skybox shaders.
                skyboxShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/skybox.glsl");

                noiseMap = Vulkyrie::Core::GeneratePerlinNoiseMap({
                    .MapWidth = 100,
                    .MapHeight = 100,
                    .Scale = scale,
                    .Octaves = 4,
                    .Persistence = 0.5f,
                    .Lacunarity = 2.0f,
                    .Seed = 42,
                });

                // Enable depth testing.
                glEnable(GL_DEPTH_TEST);
            }

            void OnUpdate(Vulkyrie::Core::Timestep deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                float cameraSpeed = 20.0f;
                auto dt = deltaTime.GetSeconds();

                if (_application.IsKeyPressed(KeyCode::LeftShift)) cameraSpeed = 100.0f;

                if (_application.IsKeyPressed(KeyCode::W)) camera.ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::S)) camera.ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::A)) camera.ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::D)) camera.ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);

                if (_application.IsKeyPressed(KeyCode::M)) {
                    if (scale < 100.0f) scale += 1.0f;

                    noiseMap = Vulkyrie::Core::GeneratePerlinNoiseMap({
                        .MapWidth = 100,
                        .MapHeight = 100,
                        .Scale = scale,
                        .Octaves = 4,
                        .Persistence = 0.5f,
                        .Lacunarity = 2.0f,
                        .Seed = 42,
                    });
                }

                if (_application.IsKeyPressed(KeyCode::N)) {
                    if (scale > 2.0f) scale -= 1.0f;

                    noiseMap = Vulkyrie::Core::GeneratePerlinNoiseMap({
                        .MapWidth = 100,
                        .MapHeight = 100,
                        .Scale = scale,
                        .Octaves = 4,
                        .Persistence = 0.5f,
                        .Lacunarity = 2.0f,
                        .Seed = 42,
                    });
                }

                // --------------------------------------------------------------------
                // Render terrain.
                terrainShader->Use();

                // View Matrix.
                glm::mat4 view = camera.GetViewMatrix();

                // Projection Matrix.
                glm::mat4 projection = glm::mat4(1.0f);
                projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);

                // glm::mat4 projection = glm::mat4(1.0f);
                // projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
                terrainShader->SetMat4Uniform("projection", projection);

                glm::mat4 viewTerrain = camera.GetViewMatrix();
                terrainShader->SetMat4Uniform("view", viewTerrain);

                // texture->Bind(0);

                vertexArray->Bind();

                for (int i = 0; i < HEIGHT; i++) {
                    const auto x = i * WIDTH;

                    for (int j = 0; j < WIDTH; j++) {
                        glm::mat4 model(1.0f);

                        glm::vec3 firstQuadrantPosition = glm::vec3(j * -1.0f, -1.0f, i * -1.0f);
                        model = glm::translate(model, firstQuadrantPosition);
                        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        terrainShader->SetMat4Uniform("model", model);

                        const glm::vec3 pixelColor = noiseMap[x + j] * glm::vec3(1.0f, 1.0f, 1.0f);
                        terrainShader->SetVec3Uniform("pixelColor", pixelColor);

                        glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
                    }
                }

                vertexArray->Unbind();
                // --------------------------------------------------------------------

                // --------------------------------------------------------------------
                // Sky box.
                // change depth function so depth test passes when values are equal to depth buffer's content
                glDepthFunc(GL_LEQUAL);
                skyboxShader->Use();

                skyboxShader->SetMat4Uniform("projection", projection);

                view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
                skyboxShader->SetMat4Uniform("view", view);

                // ... set view and projection matrix
                // glBindVertexArray(skyboxVAO);
                skyboxVertexArray->Bind();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureId);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                skyboxVertexArray->Unbind();
                // set depth function back to default
                glDepthFunc(GL_LESS);
                // --------------------------------------------------------------------
            }

            void OnEvent(Vulkyrie::Events::Event &event) override {
                Vulkyrie::Events::EventDispatcher dispatcher(event);

                dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](const Vulkyrie::Events::WindowResizedEvent &e) {
                    auto ev = static_cast<Vulkyrie::Events::WindowResizedEvent>(e);

                    glm::mat4 projection = glm::mat4(1.0f);
                    projection = glm::perspective(glm::radians(45.0f), (float)ev.Width / (float)ev.Height, 0.1f, 100.0f);

                    terrainShader->SetMat4Uniform("projection", projection);

                    return true;
                });

                dispatcher.Dispatch<Vulkyrie::Events::MouseMovedEvent>([this](const Vulkyrie::Events::MouseMovedEvent &e) {
                    auto mouseMovedEvent = static_cast<Vulkyrie::Events::MouseMovedEvent>(e);

                    if (firstMouseMove) {
                        lastMouseX = mouseMovedEvent.MouseX;
                        lastMouseY = mouseMovedEvent.MouseY;
                        firstMouseMove = false;
                    }

                    const float xOffset = mouseMovedEvent.MouseX - lastMouseX;
                    const float yOffset = lastMouseY - mouseMovedEvent.MouseY;

                    camera.ProcessMouseMovement(xOffset, yOffset);

                    lastMouseX = mouseMovedEvent.MouseX;
                    lastMouseY = mouseMovedEvent.MouseY;

                    return true;
                });
            }

        private:
            Camera camera;
            f32 windowWidth;
            f32 windowHeight;
            bool firstMouseMove = true;
            float lastMouseX = 0.0f;
            float lastMouseY = 0.0f;
            Ref<Shader> terrainShader;
            Ref<VertexArray> vertexArray;
            std::vector<f32> noiseMap;
            // Ref<Texture2D> texture;
            Ref<Shader> skyboxShader;
            Ref<VertexArray> skyboxVertexArray;
            u32 skyboxTextureId;
            f32 scale = 5.0f;

            unsigned int loadCubemap(std::vector<std::string> faces) {
                unsigned int textureID;
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

                int width, height, nrChannels;
                for (unsigned int i = 0; i < faces.size(); i++) {
                    unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
                    if (data) {
                        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                        stbi_image_free(data);
                    } else {
                        std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                        stbi_image_free(data);
                    }
                }
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

                return textureID;
            }
    };
} // namespace Pong
