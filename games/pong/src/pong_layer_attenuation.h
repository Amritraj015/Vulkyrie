#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Pong {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;

    class PongLayerAttenuation : public Vulkyrie::Core::Layer {
        public:
            PongLayerAttenuation(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application), windowWidth(windowWidth), windowHeight(windowHeight), camera(glm::vec3(0.0f, 0.0f, 5.0f)) {
                pointLight = {
                    .Light = {
                        { 0.2f, 0.2f, 0.2f },
                        { 0.5f, 0.5f, 0.5f },
                        { 1.0f, 1.0f, 1.0f },
                    },
                    .Position = { 1.2f, 1.0f, 2.0f },
                    .AttenuationConstant = 1.0f,
                    .AttenuationLinear = 0.09f,
                    .AttenuationQuadratic = 0.032f,
                };

                // Starting vertices.
                vertices = {
                    // positions         // normals    // texture coords
                    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, //
                    0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 0.0f, //
                    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f, //
                    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f, //
                    -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 1.0f, //
                    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, //

                    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, //
                    0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, //
                    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, //
                    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, //
                    -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, //
                    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, //

                    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, //
                    -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,  1.0f, 1.0f, //
                    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, //
                    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, //
                    -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f, 0.0f, //
                    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, //

                    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, //
                    0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,  1.0f, 1.0f, //
                    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f, //
                    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f, //
                    0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, //
                    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, //

                    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, //
                    0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f, 1.0f, //
                    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f, //
                    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f, //
                    -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f, 0.0f, //
                    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, //

                    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f, //
                    0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  1.0f, 1.0f, //
                    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, //
                    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, //
                    -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, //
                    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f  //
                };

                // load and compile the shader programs.
                objectShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/attenuation.glsl");
                lightShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/light-source.glsl");

                // Check if shaders are loaded successfully.
                if (!objectShader->IsValid() || !lightShader->IsValid()) {
                    VERROR("Failed to load shaders.");
                    return;
                }

                // load the textures.
                boxTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/container2.png");
                specularMapTexture = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/container2_specular.png");

                // Check if textures are loaded successfully.
                if (!boxTexture->IsLoaded() || !specularMapTexture->IsLoaded()) {
                    VERROR("Failed to load one or more textures!");
                    return;
                }

                // Create Vertex Array.
                objectVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);

                // Create Vertex Buffer.
                objectVertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, vertices.data(), vertices.size() * sizeof(f32));

                // Set layout for the vertex buffer.
                objectVertexBuffer->SetLayout({
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aPos" },
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aNormal" },
                    { Vulkyrie::Renderer::ShaderDataType::Float2, "aTexture" },
                });

                // Add Vertex Buffer to the vertex array.
                objectVertexArray->AddVertexBuffer(objectVertexBuffer);

                // Create the vertex array for the light source.
                lightVertexArray = VertexArray::Create(GraphicsAPI::OpenGL);

                // Reuse the same vertex buffer for the light source.
                lightVertexArray->AddVertexBuffer(objectVertexBuffer);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            void OnAttach() override {
                VDEBUG("Layer Attached: Pong Layer Attenuation.");
            }

            void OnDetach() override {
                VDEBUG("Layer Detached: Pong Layer Attenuation.");
            }

            void OnUpdate(const Timestep deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                constexpr f32 cameraSpeed = 5.0f;
                auto dt = deltaTime.GetSeconds();

                if (_application.IsKeyPressed(KeyCode::W)) camera.ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::S)) camera.ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::A)) camera.ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::D)) camera.ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);

                objectShader->Use();
                objectShader->SetVec3Uniform("viewPos", camera.GetPosition());

                // projection transformations.
                glm::mat4 projection = glm::perspective(glm::radians(45.0f), (f32)windowWidth / (f32)windowHeight, 0.1f, 100.0f);
                objectShader->SetMat4Uniform("projection", projection);

                // view matrix.
                glm::mat4 view = camera.GetViewMatrix();
                objectShader->SetMat4Uniform("view", view);

                // world transformation.
                glm::mat4 model = glm::mat4(1.0f);
                objectShader->SetMat4Uniform("model", model);

                // Set the material properties for the object.
                objectShader->SetIntUniform("material.diffuse", 0);
                objectShader->SetIntUniform("material.specular", 1);
                objectShader->SetFloatUniform("material.shininess", 32.0f);

                // Bind the textures to texture units.
                boxTexture->Bind(0);
                specularMapTexture->Bind(1);

                pointLight.Position = glm::vec3(2.0f * sin(glfwGetTime()), 0.0f, 5.0f * cos(glfwGetTime()));

                // Set the light properties.
                objectShader->SetVec3Uniform("light.position", pointLight.Position);
                objectShader->SetVec3Uniform("light.ambient", pointLight.Light.Ambient);
                objectShader->SetVec3Uniform("light.diffuse", pointLight.Light.Diffuse);
                objectShader->SetVec3Uniform("light.specular", pointLight.Light.Specular);

                // Set attenuation factors.
                objectShader->SetFloatUniform("light.constant", pointLight.AttenuationConstant);
                objectShader->SetFloatUniform("light.linear", pointLight.AttenuationLinear);
                objectShader->SetFloatUniform("light.quadratic", pointLight.AttenuationQuadratic);

                // Issue a draw call to draw the reflecting object.
                objectVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                objectVertexArray->Unbind();

                // -----------------------------------------------------------------------------------
                // also draw the lamp object
                lightShader->Use();
                lightShader->SetMat4Uniform("projection", projection);
                lightShader->SetMat4Uniform("view", view);
                model = glm::translate(model, pointLight.Position);
                model = glm::scale(model, glm::vec3(0.1f)); // a smaller cube
                lightShader->SetMat4Uniform("model", model);

                // Issue a draw call to draw the light source.
                lightVertexArray->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                lightVertexArray->Unbind();
            }

            void OnEvent(Vulkyrie::Events::Event &event) override {
                Vulkyrie::Events::EventDispatcher dispatcher(event);

                dispatcher.Dispatch<Vulkyrie::Events::MouseMovedEvent>([this](const Vulkyrie::Events::MouseMovedEvent &e) {
                    auto mouseMovedEvent = static_cast<Vulkyrie::Events::MouseMovedEvent>(e);

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
            Ref<VertexArray> objectVertexArray;
            Ref<VertexBuffer> objectVertexBuffer;
            Ref<Shader> objectShader;

            Ref<VertexArray> lightVertexArray;
            Ref<VertexBuffer> lightVertexBuffer;
            Ref<Shader> lightShader;

            Ref<Texture2D> boxTexture;
            Ref<Texture2D> specularMapTexture;

            Camera camera;
            f64 lastMouseX = 400.0f;
            f64 lastMouseY = 300.0f;
            bool firstMouseMove = true;
            f32 windowHeight, windowWidth;
            std::vector<f32> vertices;
            PointLight pointLight;
    };

} // namespace Pong
