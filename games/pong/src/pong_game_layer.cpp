#include "pong_game_layer.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "pong_overlay_layer.h"

namespace Pong {
    static constexpr float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, //
        0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, //
        0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, //
        0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, //
        -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, //
        -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, //

        -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, //
        0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, //
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, //
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, //
        -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f, //
        -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, //

        -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f, //
        -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f, //
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f, //
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f, //
        -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f, //
        -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f, //

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, //
        0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f, //
        0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, //
        0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, //
        0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f, //
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, //

        -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, //
        0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, //
        0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, //
        0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, //
        -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, //
        -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, //

        -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f, //
        0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f, //
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, //
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, //
        -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f, //
        -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f  //
    };

    static constexpr unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    static glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

    PongGameLayer::PongGameLayer(const Vulkyrie::Core::Application &application, f32 windowWidth, f32 windowHeight)
        : Vulkyrie::Core::Layer(application), windowWidth(windowWidth), windowHeight(windowHeight),
          objectShader("assets/shaders/reflective-object.vert.glsl", "assets/shaders/reflective-object.frag.glsl"),
          lightShader("assets/shaders/light-source.vert.glsl", "assets/shaders/light-source.frag.glsl"),
          camera(glm::vec3(0.0f, 0.0f, 3.0f)) {

        if (!objectShader.IsValid() || !lightShader.IsValid()) {
            VERROR("Failed to load shaders.");
            return;
        }

        // Create Vertex Array.
        objectVertexArray = VertexArray::Create(Vulkyrie::Core::GraphicsAPI::OpenGL);

        // Create Vertex Buffer.
        objectVertexBuffer = VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, const_cast<float *>(vertices), sizeof(vertices));

        // Set layout for the vertex buffer.
        objectVertexBuffer->SetLayout({
            { Vulkyrie::Renderer::ShaderDataType::Float3, "a_Position" },
            { Vulkyrie::Renderer::ShaderDataType::Float3, "aNormal" },
        });

        // Add Vertex Buffer to the vertex array.
        objectVertexArray->AddVertexBuffer(objectVertexBuffer);

        // Create the vertex array for the light source.
        lightVertexArray = VertexArray::Create(Vulkyrie::Core::GraphicsAPI::OpenGL);

        // Create the vertex buffer for the light source.
        // lightVertexBuffer = VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, const_cast<float *>(vertices), sizeof(vertices));
        // lightVertexBuffer->SetLayout({
        //     { Vulkyrie::Renderer::ShaderDataType::Float3, "a_Position" },
        //     { Vulkyrie::Renderer::ShaderDataType::Float3, "aNormal" },
        // });

        // Reuse the same vertex buffer for the light source.
        lightVertexArray->AddVertexBuffer(objectVertexBuffer);

        // This is required to make sure 3D rendering works properly.
        glEnable(GL_DEPTH_TEST);
    }

    void PongGameLayer::OnAttach() {
        VDEBUG("Layer Attached: Pong Game Layer.");
    }

    void PongGameLayer::OnDetach() {
        VDEBUG("Layer Detached: Pong Game Layer.");
    }

    void PongGameLayer::OnUpdate(Vulkyrie::Core::Timestep deltaTime) {
        dt = deltaTime.GetSeconds();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        objectShader.Use();
        objectShader.SetVec3Uniform("objectColor", 1.0f, 0.5f, 0.31f);
        objectShader.SetVec3Uniform("lightColor", 1.0f, 1.0f, 1.0f);

        objectShader.SetVec3Uniform("lightPos", lightPos);  
        objectShader.SetVec3Uniform("viewPos", camera.GetPosition()); 


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        objectShader.SetMat4Uniform("projection", projection);
        objectShader.SetMat4Uniform("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        objectShader.SetMat4Uniform("model", model);

        // Issue a draw call to draw the light source.
        objectVertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // -----------------------------------------------------------------------------------
        // also draw the lamp object
        lightShader.Use();
        lightShader.SetMat4Uniform("projection", projection);
        lightShader.SetMat4Uniform("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        lightShader.SetMat4Uniform("model", model);

        // graphicsShader.Use();
        lightVertexArray->Bind();

        // Issue a draw call to draw the reflecting object.
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    void PongGameLayer::OnEvent(Vulkyrie::Events::Event &event) {
        Vulkyrie::Events::EventDispatcher dispatcher(event);

        dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](const Vulkyrie::Events::WindowResizedEvent &e) {
            auto ev = static_cast<Vulkyrie::Events::WindowResizedEvent>(e);

            glm::mat4 projection = glm::mat4(1.0f);
            projection = glm::perspective(glm::radians(45.0f), (float)ev.Width / (float)ev.Height, 0.1f, 100.0f);

            // graphicsShader.SetMat4Uniform("projection", projection);

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

        dispatcher.Dispatch<Vulkyrie::Events::KeyPressedEvent>([this](const Vulkyrie::Events::KeyPressedEvent &e) {
            constexpr float cameraSpeed = 30.0f; // adjust accordingly

            if (e.KeyCode == Vulkyrie::Events::KeyCode::W)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::FORWARD, cameraSpeed, dt);
            else if (e.KeyCode == Vulkyrie::Events::KeyCode::S)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::BACKWARD, cameraSpeed, dt);
            else if (e.KeyCode == Vulkyrie::Events::KeyCode::A)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::LEFT, cameraSpeed, dt);
            else if (e.KeyCode == Vulkyrie::Events::KeyCode::D)
                camera.ProcessKeyboardMovement(Vulkyrie::Renderer::RIGHT, cameraSpeed, dt);
            else if (e.KeyCode == Vulkyrie::Events::KeyCode::J) {
                _application.PushLayer<Pong::PongOverlayLayer>(windowWidth, windowHeight);
                // _application.PushLayer<Pong::PongOverlayLayer>(windowWidth, windowHeight);
            }

            return false;
        });
    }

} // namespace Pong
