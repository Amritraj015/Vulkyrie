#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include "model.h"
#include <GLFW/glfw3.h>

namespace Pong {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Renderer;

    class PongLayerPlanet final : public Layer {
        public:
            PongLayerPlanet(Application &application, f32 windowWidth, f32 windowHeight)
                : Layer(application), windowWidth(windowWidth), windowHeight(windowHeight), camera(glm::vec3(0.0f, 50.0f, 1000.0f)) {
                planetModel = CreateRef<Model>("assets/models/planet/planet.obj");
                asteroidModel = CreateRef<Model>("assets/models/asteroid/rock.obj");

                planetShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/model.glsl");
                asteroidShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/asteroid.glsl");

                if (!planetShader->IsValid() || !asteroidShader->IsValid()) {
                    VERROR("Failed to compile shaders.");
                }

                amount = 1000000;
                modelMatrices.reserve(amount);
                srand(glfwGetTime()); // initialize random seed
                f32 radius = 500.0;
                f32 offset = 50.0f;

                for (u32 i = 0; i < amount; i++) {
                    glm::mat4 model = glm::mat4(1.0f);
                    // 1. translation: displace along circle with 'radius' in range [-offset, offset]
                    f32 angle = (f32)i / (f32)amount * 360.0f;
                    f32 displacement = (rand() % (i32)(2 * offset * 100)) / 100.0f - offset;
                    f32 x = sin(angle) * radius + displacement;
                    displacement = (rand() % (i32)(2 * offset * 100)) / 100.0f - offset;
                    f32 y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
                    displacement = (rand() % (i32)(2 * offset * 100)) / 100.0f - offset;
                    f32 z = cos(angle) * radius + displacement;
                    model = glm::translate(model, glm::vec3(x, y, z));

                    // 2. scale: scale between 0.05 and 0.25f
                    f32 scale = (rand() % 20) / 100.0f + 0.05;
                    model = glm::scale(model, glm::vec3(scale));

                    // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
                    f32 rotAngle = (rand() % 360);
                    model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

                    // 4. now add to list of matrices
                    modelMatrices.push_back(model);
                }

                // configure instanced array
                // -------------------------
                u32 buffer;
                glGenBuffers(1, &buffer);
                glBindBuffer(GL_ARRAY_BUFFER, buffer);
                glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

                // set transformation matrices as an instance vertex attribute (with divisor 1)
                // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
                // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
                // -----------------------------------------------------------------------------------------------------------------------------------
                for (u32 i = 0; i < asteroidModel->meshes.size(); i++) {
                    u32 VAO = asteroidModel->meshes[i].VAO;
                    glBindVertexArray(VAO);

                    // set attribute pointers for matrix (4 times vec4)
                    glEnableVertexAttribArray(3);
                    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)0);
                    glEnableVertexAttribArray(4);
                    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(glm::vec4)));
                    glEnableVertexAttribArray(5);
                    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(2 * sizeof(glm::vec4)));
                    glEnableVertexAttribArray(6);
                    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(3 * sizeof(glm::vec4)));

                    glVertexAttribDivisor(3, 1);
                    glVertexAttribDivisor(4, 1);
                    glVertexAttribDivisor(5, 1);
                    glVertexAttribDivisor(6, 1);

                    glBindVertexArray(0);
                }

                glEnable(GL_DEPTH_TEST);
            }

            void OnUpdate(const Timestep deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                f32 cameraSpeed = 100.0f;
                auto dt = deltaTime.GetSeconds();

                if (_application.IsKeyPressed(KeyCode::LeftShift)) cameraSpeed = 500.0f;

                if (_application.IsKeyPressed(KeyCode::W)) camera.ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::S)) camera.ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::A)) camera.ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
                if (_application.IsKeyPressed(KeyCode::D)) camera.ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);

                planetShader->Use();

                // view/projection transformations
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (f32)windowWidth / (f32)windowHeight, 0.1f, 20000.0f);
                glm::mat4 view = camera.GetViewMatrix();
                planetShader->SetMat4Uniform("projection", projection);
                planetShader->SetMat4Uniform("view", view);

                // render the loaded model
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians((f32)glfwGetTime() * -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, glm::vec3(70.0f, 70.0f, 70.0f));
                planetShader->SetMat4Uniform("model", model);

                // draw planet
                planetModel->Draw(*planetShader);

                // draw meteorites
                asteroidShader->Use();
                asteroidShader->SetMat4Uniform("projection", projection);
                asteroidShader->SetMat4Uniform("view", view);

                // Apply orbital rotation to all asteroids
                glm::mat4 orbitRotation = glm::rotate(glm::mat4(1.0f), glm::radians((f32)glfwGetTime() * 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                asteroidShader->SetMat4Uniform("orbitRotation", orbitRotation);

                asteroidShader->SetIntUniform("texture_diffuse", 0);
                glActiveTexture(GL_TEXTURE0);

                // note: we also made the textures_loaded vector public (instead of private) from the model class.
                glBindTexture(GL_TEXTURE_2D, asteroidModel->textures_loaded[0].id);
                for (u32 i = 0; i < asteroidModel->meshes.size(); i++) {
                    glBindVertexArray(asteroidModel->meshes[i].VAO);
                    glDrawElementsInstanced(GL_TRIANGLES, static_cast<u32>(asteroidModel->meshes[i].indices.size()), GL_UNSIGNED_INT, 0, amount);
                    glBindVertexArray(0);
                }
            }

            void OnAttach() override {
                VDEBUG("Layer Detached: Pong Layer Planet.");
            }

            void OnDetach() override {
                VDEBUG("Layer Detached: Pong Layer Planet.");
            }

            void OnEvent(Vulkyrie::Events::Event &event) override {
                Vulkyrie::Events::EventDispatcher dispatcher(event);

                dispatcher.Dispatch<Vulkyrie::Events::MouseScrolledEvent>([this](const Vulkyrie::Events::MouseScrolledEvent &e) {
                    auto scrollEvent = static_cast<MouseScrolledEvent>(e);
                    camera.ProcessMouseScroll(scrollEvent.OffsetY);

                    return true;
                });

                dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](const Vulkyrie::Events::WindowResizedEvent &e) {
                    auto ev = static_cast<Vulkyrie::Events::WindowResizedEvent>(e);
                    glm::mat4 projection = glm::mat4(1.0f);
                    projection = glm::perspective(glm::radians(45.0f), (f32)ev.Width / (f32)ev.Height, 0.1f, 100.0f);

                    return true;
                });

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
            };

        private:
            Ref<Model> planetModel;
            Ref<Model> asteroidModel;
            Ref<Shader> planetShader;
            Ref<Shader> asteroidShader;
            f32 windowHeight;
            f32 windowWidth;
            Camera camera;
            bool firstMouseMove = true;
            f64 lastMouseX = 400.0f;
            f64 lastMouseY = 300.0f;
            u32 amount;
            std::vector<glm::mat4> modelMatrices;
    };
} // namespace Pong
