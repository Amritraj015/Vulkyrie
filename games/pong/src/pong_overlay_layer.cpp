#include "pong_overlay_layer.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Pong {
    constexpr float vertices[] = {
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
        -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, //
    };

    constexpr unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glm::vec3 cubePositions[] = {
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

    // Ref<Texture2D> texture1;
    // Ref<Texture2D> texture2;
    // Ref<VertexBuffer> vertexBuffer;
    // Ref<VertexArray> vertexArray;
    // GraphicsShader graphicsShader;
    // Camera camera;
    float dt = 0.0f; // Time between current frame and last frame

    PongOverlayLayer::PongOverlayLayer(const std::string_view layerName)
        : Layer(layerName), graphicsShader("assets/shaders/triangle.vert.glsl", "assets/shaders/triangle.frag.glsl") {
        // Check if shader program creation failed.
        if (!graphicsShader.IsValid()) {
            // Log a fatal error.
            VFATAL("Failed to create graphics shader");

            return;
        }

        vertexArray = VertexArray::Create(GraphicsAPI::OpenGL);
        vertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, const_cast<float *>(vertices), sizeof(vertices));
        vertexBuffer->SetLayout({
            { ShaderDataType::Float3, "aPos" },
            { ShaderDataType::Float2, "aTexCoord" },
        });
        vertexArray->AddVertexBuffer(vertexBuffer);

        const auto indexBuffer = IndexBuffer::Create(GraphicsAPI::OpenGL, const_cast<u32 *>(indices), sizeof(indices) / sizeof(u32));
        vertexArray->SetIndexBuffer(indexBuffer);

        // -----------------------------------------------
        // Textures.
        texture1 = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/wall.jpg");
        texture2 = Texture2D::Create(GraphicsAPI::OpenGL, "assets/textures/awesomeface.png");

        if (!texture1->IsLoaded() || !texture2->IsLoaded()) {
            VERROR("Failed to load one or more textures!");
        }

        graphicsShader.Use();
        graphicsShader.SetIntUniform("texture1", 0);
        graphicsShader.SetIntUniform("texture2", 1);
        glEnable(GL_DEPTH_TEST);

        // -----------------------------------------------
    }

    void PongOverlayLayer::OnAttach() {
        VDEBUG("Layer Attached: {}", _layerName.data());
    }

    void PongOverlayLayer::OnDetach() {
        VDEBUG("Layer Detached: {}", _layerName.data());
    }

    void PongOverlayLayer::OnUpdate(Vulkyrie::Core::Timestep deltaTime) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the color and depth buffer

        // bind Texture
        // texture1->Bind(GL_TEXTURE0);

        // GLenum err = glGetError();
        // if (err != GL_NO_ERROR) VERROR("Error Binding texture 1: {}", err);

        // texture2->Bind(GL_TEXTURE1);

        // err = glGetError();
        // if (err != GL_NO_ERROR) VERROR("Error Binding texture 2: {}", err);

        dt = deltaTime.GetSeconds();

        // or more simply:
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1->GetTextureID());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2->GetTextureID());

        // create transformations
        graphicsShader.Use();

        auto view = camera.GetViewMatrix();
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        // retrieve the matrix uniform locations
        // unsigned int modelLoc = glGetUniformLocation(graphicsShader.GetShaderProgram(), "model");
        // unsigned int viewLoc = glGetUniformLocation(graphicsShader.GetShaderProgram(), "view");

        // pass them to the shaders (3 different ways)
        // glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        // glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);

        // note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best
        // practice to set it outside the main loop only once.
        graphicsShader.SetMat4Uniform("view", view);
        graphicsShader.SetMat4Uniform("projection", projection);

        // render container
        // Use the graphics shader program.
        // glBindVertexArray(vao);
        vertexArray->Bind();
        // glDrawArrays(GL_TRIANGLES, 0, 36);

        for (unsigned int i = 0; i < 10; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * (i + 1);

            model = glm::rotate(model, (float)glfwGetTime() * glm::radians(angle), glm::vec3(0.5f, 1.0f, 0.0f));
            graphicsShader.SetMat4Uniform("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    void PongOverlayLayer::OnEvent(Vulkyrie::Events::Event &event) {
        // VINFO("{} - Event: {}",  _layerName.c_str(), event.ToString());
        Vulkyrie::Events::EventDispatcher dispatcher(event);

        dispatcher.Dispatch<Vulkyrie::Events::MouseMovedEvent>([this](const Vulkyrie::Events::MouseMovedEvent &e) {
            auto mouseMovedEvent = static_cast<Vulkyrie::Events::MouseMovedEvent>(e);

            if (firstMouseMove) {
                lastMouseX = mouseMovedEvent.GetX();
                lastMouseY = mouseMovedEvent.GetY();
                firstMouseMove = false;
            }

            const float xOffset = mouseMovedEvent.GetX() - lastMouseX;
            const float yOffset = lastMouseY - mouseMovedEvent.GetY();

            camera.ProcessMouseMovement(xOffset, yOffset);

            lastMouseX = mouseMovedEvent.GetX();
            lastMouseY = mouseMovedEvent.GetY();

            return true;
        });

        dispatcher.Dispatch<Vulkyrie::Events::KeyPressedEvent>([this](const Vulkyrie::Events::KeyPressedEvent &e) {
            // if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::J) {
            //     _toggleWireframe = !_toggleWireframe;

            //     VINFO("J key pressed in {}!", _layerName.data());
            //     // Vulkyrie::Core::ApplicationManager::ToggleWireframeMode(_toggleWireframe);

            //     return true;
            // }

            constexpr float cameraSpeed = 30.0f; // adjust accordingly

            if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::W)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::FORWARD, cameraSpeed, dt);
            else if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::S)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::BACKWARD, cameraSpeed, dt);
            else if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::A)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::LEFT, cameraSpeed, dt);
            else if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::D)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::RIGHT, cameraSpeed, dt);

            return false;
        });
    }

} // namespace Pong
