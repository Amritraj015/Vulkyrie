#pragma once

#include <vulkyrie.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Sandbox {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;
    using namespace Vulkyrie::Renderer;

    class SandboxLayerPlanet final : public Layer {
        public:
            SandboxLayerPlanet()
                : camera(Camera::Create()) {
                planetModel = Model::Create("assets/models/planet/planet.obj");
                asteroidModel = Model::Create("assets/models/asteroid/rock.obj");

                planetShader = Shader::Create("assets/shaders/model.glsl");
                asteroidShader = Shader::Create("assets/shaders/asteroid.glsl");

                if (!planetShader->IsValid() || !asteroidShader->IsValid()) {
                    VERROR("Failed to compile shaders.")
                }

                camera.SetMovementSpeed(50.0f, 100.0f, 500.0f);

                amount = 100000;
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
                for (u32 i = 0; i < asteroidModel->GetMeshCount(); i++) {
                    const auto &mesh = asteroidModel->GetMeshes()[i];
                    mesh->Bind();

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

                    mesh->Unbind();
                }

                // Enable depth testing for correct 3D rendering.
                glEnable(GL_DEPTH_TEST);

                // Enable face culling to improve performance.
                glEnable(GL_CULL_FACE);
            }

            void OnResumed() override { glEnable(GL_CULL_FACE); }
            void OnSuspended() override { glDisable(GL_CULL_FACE); }

            void OnUpdate(const Timestep &deltaTime) override {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                camera.OnUpdate(deltaTime);

                planetShader->Use();

                // view/projection transformations
                glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                        (f32)Application::GetSingleton().GetWindowWidth() / (f32)Application::GetSingleton().GetWindowHeight(),
                                                        0.1f,
                                                        5000.0f);
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

                asteroidShader->SetIntUniform("texture_diffuse1", 0);
                // glActiveTexture(GL_TEXTURE0);

                // note: we also made the textures_loaded vector public (instead of private) from the model class.
                // TODO: Remove the following line maybe.
                // glBindTexture(GL_TEXTURE_2D, asteroidModel->_loadedTextures[0].Id);
                asteroidModel->BindTextures();
                for (u32 i = 0; i < asteroidModel->GetMeshCount(); i++) {
                    const auto &mesh = asteroidModel->GetMeshes()[i];
                    mesh->Bind();
                    glDrawElementsInstanced(GL_TRIANGLES, static_cast<u32>(mesh->GetIndexCount()), GL_UNSIGNED_INT, 0, amount);
                    mesh->Unbind();
                }

// Check for OpenGL errors (optional, for debugging)
#ifdef VULKYRIE_DEBUG
                GLenum err;
                while ((err = glGetError()) != GL_NO_ERROR) {
                    VERROR("OpenGL error: {}", err);
                }
#endif
            }

            void OnAttached() override { VDEBUG("Layer Attached: Planet"); }
            void OnDetached() override { VDEBUG("Layer Detached: Planet"); }

            void OnEvent(Vulkyrie::Events::Event &event) override {
                Vulkyrie::Events::EventDispatcher dispatcher(event);

                dispatcher.Dispatch<Vulkyrie::Events::MouseScrolledEvent>([this](const Vulkyrie::Events::MouseScrolledEvent &e) {
                    camera.ProcessMouseScroll(e.OffsetY);

                    return true;
                });
            };

        private:
            Ref<Model> planetModel;
            Ref<Model> asteroidModel;
            Ref<Shader> planetShader;
            Ref<Shader> asteroidShader;
            Camera camera;
            u32 amount;
            std::vector<glm::mat4> modelMatrices;
    };
} // namespace Sandbox
