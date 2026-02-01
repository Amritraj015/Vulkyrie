#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Renderer;

    class SandboxLayerAttenuation : public Vulkyrie::Core::Layer {
        public:
            SandboxLayerAttenuation()
                : camera(Camera::Create()) {

                // load and compile the shader programs.
                // objectShader = Shader::Create("assets/shaders/attenuation.glsl");
                objectShader = Shader::Create("assets/shaders/spotlight.glsl");
                // lightShader = Shader::Create("assets/shaders/light-source.glsl");

                // Check if shaders are loaded successfully.
                // if (!objectShader->IsValid() || !lightShader->IsValid()) {
                if (!objectShader->IsValid()) {
                    VERROR("Failed to load shaders.");
                    return;
                }

                // load the textures.
                boxTexture = Texture2D::Create("assets/textures/container2.png");
                specularMapTexture = Texture2D::Create("assets/textures/container2_specular.png");

                // Check if textures are loaded successfully.
                if (!boxTexture->IsLoaded() || !specularMapTexture->IsLoaded()) {
                    VERROR("Failed to load one or more textures!");
                    return;
                }

                // Create Vertex Array.
                objectVertexArray = VertexArray::Create();

                // Create Vertex Buffer.
                objectVertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(f32));

                // Set layout for the vertex buffer.
                objectVertexBuffer->SetLayout({
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aPos" },
                    { Vulkyrie::Renderer::ShaderDataType::Float3, "aNormal" },
                    { Vulkyrie::Renderer::ShaderDataType::Float2, "aTexture" },
                });

                // Add Vertex Buffer to the vertex array.
                objectVertexArray->AddVertexBuffer(objectVertexBuffer);

                // Create the vertex array for the light source.
                lightVertexArray = VertexArray::Create();

                // Reuse the same vertex buffer for the light source.
                lightVertexArray->AddVertexBuffer(objectVertexBuffer);

                // This is required to make sure 3D rendering works properly.
                glEnable(GL_DEPTH_TEST);
            }

            void OnAttached() override {
                VDEBUG("Layer Attached: Attenuation");
            }

            void OnDetached() override {
                VDEBUG("Layer Detached: Attenuation");
            }

            void OnUpdate(Timestep deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update Camera position based on input.
                camera.OnUpdate(deltaTime);

                objectShader->Use();
                objectShader->SetVec3Uniform("viewPos", camera.GetPosition());

                // projection transformations.
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        1000.0f);
                objectShader->SetMat4Uniform("projection", projection);

                // view matrix.
                glm::mat4 view = camera.GetViewMatrix();
                objectShader->SetMat4Uniform("view", view);

                // Set the material properties for the object.
                objectShader->SetIntUniform("material.diffuse", 0);
                objectShader->SetIntUniform("material.specular", 1);
                objectShader->SetFloatUniform("material.shininess", 32.0f);

                // Bind the textures to texture units.
                boxTexture->Bind(0);
                specularMapTexture->Bind(1);

                // Update spotlight position and direction to match the camera.
                spotLight.Position = camera.GetPosition();
                spotLight.Direction = camera.GetFront();

                // Set the spotlight properties.
                objectShader->SetVec3Uniform("light.position", spotLight.Position);
                objectShader->SetVec3Uniform("light.ambient", spotLight.Ambient);
                objectShader->SetVec3Uniform("light.diffuse", spotLight.Diffuse);
                objectShader->SetVec3Uniform("light.specular", spotLight.Specular);
                objectShader->SetFloatUniform("light.constant", spotLight.AttenuationConstant);
                objectShader->SetFloatUniform("light.linear", spotLight.AttenuationLinear);
                objectShader->SetFloatUniform("light.quadratic", spotLight.AttenuationQuadratic);
                objectShader->SetVec3Uniform("light.direction", spotLight.Direction);
                objectShader->SetFloatUniform("light.cutoffInner", spotLight.CutoffInner);
                objectShader->SetFloatUniform("light.cutoffOuter", spotLight.CutoffOuter);

                // Issue a draw call to draw the reflecting object.
                objectVertexArray->Bind();

                for (const auto location : cubePositions) {
                    // world transformation.
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, location);
                    objectShader->SetMat4Uniform("model", model);

                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                objectVertexArray->Unbind();

                // -----------------------------------------------------------------------------------
                // also draw the lamp object
                // pointLight.Position = glm::vec3(5.0f * sin(glfwGetTime()), 0.0f, 5.0f * cos(glfwGetTime()));
                //
                // lightShader->Use();
                // lightShader->SetMat4Uniform("projection", projection);
                // lightShader->SetMat4Uniform("view", view);
                // glm::mat4 model = glm::mat4(1.0f);
                // model = glm::translate(model, pointLight.Position);
                // model = glm::scale(model, glm::vec3(0.1f)); // a smaller cube
                // lightShader->SetMat4Uniform("model", model);
                //
                // // Issue a draw call to draw the light source.
                // lightVertexArray->Bind();
                // glDrawArrays(GL_TRIANGLES, 0, 36);
                // lightVertexArray->Unbind();
            }

        private:
            Ref<VertexArray> objectVertexArray;
            Ref<VertexBuffer> objectVertexBuffer;
            Ref<Shader> objectShader;

            Ref<VertexArray> lightVertexArray;
            Ref<VertexBuffer> lightVertexBuffer;
            // Ref<Shader> lightShader;

            Ref<Texture2D> boxTexture;
            Ref<Texture2D> specularMapTexture;

            Camera camera;
            SpotLight spotLight = {
                { 0.1f, 0.1f, 0.1f },
                { 0.8f, 0.8f, 0.8f },
                {
                    1.0f,
                    1.0f,
                    1.0f,
                },
                camera.GetPosition(),
                1.0f,
                0.022f,
                0.0019f,
                camera.GetFront(),
                glm::cos(glm::radians(12.5f)),
                glm::cos(glm::radians(17.5f)),
            };

            // PointLight pointLight = {
            //     {
            //         0.2f,
            //         0.2f,
            //         0.2f,
            //     },
            //     {
            //         0.5f,
            //         0.5f,
            //         0.5f,
            //     },
            //     {
            //         1.0f,
            //         1.0f,
            //         1.0f,
            //     },
            //     {
            //         1.2f,
            //         1.0f,
            //         2.0f,
            //     },
            //     1.0f,
            //     0.09f,
            //     0.032f,
            // };

            std::vector<f32> vertices = {
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

            std::vector<glm::vec3> cubePositions = {
                glm::vec3(0.0f, 0.0f, 0.0f),     // Cube 1
                glm::vec3(2.0f, 5.0f, -15.0f),   // Cube 2
                glm::vec3(-1.5f, -2.2f, -2.5f),  // Cube 3
                glm::vec3(-3.8f, -2.0f, -12.3f), // Cube 4
                glm::vec3(2.4f, -0.4f, -3.5f),   // Cube 5
                glm::vec3(-1.7f, 3.0f, -7.5f),   // Cube 6
                glm::vec3(1.3f, -2.0f, -2.5f),   // Cube 7
                glm::vec3(1.5f, 2.0f, -2.5f),    // Cube 8
                glm::vec3(1.5f, 0.2f, -1.5f),    // Cube 9
                glm::vec3(-1.3f, 1.0f, -1.5f)    // Cube 10
            };
    };

} // namespace Sandbox
