#include "pong_layer_1.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Pong {
    using namespace Vulkyrie::Events;

    constexpr f32 vertices[] = {
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

    PongLayer1::PongLayer1(Application &application, f32 windowWidth, f32 windowHeight)
        : Layer(application), windowWidth(windowWidth), windowHeight(windowHeight), camera(glm::vec3(0.0f, 0.0f, 5.0f)) {

        // Load and compile shader program.
        graphicsShader = Shader::Create(GraphicsAPI::OpenGL, "assets/shaders/triangle.glsl");

        // Check if shader program creation failed.
        if (!graphicsShader->IsValid()) {
            // Log a fatal error.
            VFATAL("Failed to create graphics shader");

            return;
        }

        vertexArray = VertexArray::Create(GraphicsAPI::OpenGL);
        vertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, const_cast<f32 *>(vertices), sizeof(vertices));
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

        // This is required to make sure 3D rendering works properly.
        glEnable(GL_DEPTH_TEST);

        // Projection matrix hardly ever changes, so it can live outside the main application loop.
        graphicsShader->Use();
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), (f32)windowWidth / (f32)windowHeight, 0.1f, 100.0f);

        graphicsShader->SetMat4Uniform("projection", projection);
    }

    void PongLayer1::OnAttach() {
        VDEBUG("Layer Attached: Pong Layer 1.");
    }

    void PongLayer1::OnDetach() {
        VDEBUG("Layer Detached: Pong Layer 1.");
    }

    void PongLayer1::OnUpdate(const Vulkyrie::Core::Timestep deltaTime) {
        // clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update Camera position based on input.
        constexpr f32 cameraSpeed = 5.0f;
        auto dt = deltaTime.GetSeconds();

        if (_application.IsKeyPressed(KeyCode::W)) camera.ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
        if (_application.IsKeyPressed(KeyCode::S)) camera.ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
        if (_application.IsKeyPressed(KeyCode::A)) camera.ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
        if (_application.IsKeyPressed(KeyCode::D)) camera.ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);

        // Use the graphics shader program.
        graphicsShader->Use();

        // bind Texture
        texture1->Bind(0);
        texture2->Bind(1);

        auto view = camera.GetViewMatrix();

        graphicsShader->SetMat4Uniform("view", view);

        // render container
        vertexArray->Bind();

        for (unsigned int i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            f32 angle = 20.0f * (i + 1);

            model = glm::rotate(model, (f32)glfwGetTime() * glm::radians(angle), glm::vec3(0.5f, 1.0f, 0.0f));
            graphicsShader->SetMat4Uniform("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    void PongLayer1::OnEvent(Vulkyrie::Events::Event &event) {
        Vulkyrie::Events::EventDispatcher dispatcher(event);

        dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](const Vulkyrie::Events::WindowResizedEvent &e) {
            auto ev = static_cast<Vulkyrie::Events::WindowResizedEvent>(e);

            windowWidth = static_cast<f32>(ev.Width);
            windowHeight = static_cast<f32>(ev.Height);

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
    }
} // namespace Pong
