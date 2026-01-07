#include "pong_layer_2.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Pong {
    using namespace Vulkyrie::Events;

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

    PongLayer2::PongLayer2(Vulkyrie::Core::Application &application, f32 windowWidth, f32 windowHeight)
        : Vulkyrie::Core::Layer(application), windowWidth(windowWidth), windowHeight(windowHeight), camera(glm::vec3(0.0f, 0.0f, 5.0f)) {

        // Load and compile shader programs.
        objectShader = GraphicsShader::Create(GraphicsAPI::OpenGL, "assets/shaders/reflective-object.vert.glsl", "assets/shaders/reflective-object.frag.glsl");
        lightShader = GraphicsShader::Create(GraphicsAPI::OpenGL, "assets/shaders/light-source.vert.glsl", "assets/shaders/light-source.frag.glsl");

        // Check if shaders are valid.
        if (!objectShader->IsValid() || !lightShader->IsValid()) {
            VERROR("Failed to load shaders.");
            return;
        }

        // Create Vertex Array.
        objectVertexArray = VertexArray::Create(Vulkyrie::Core::GraphicsAPI::OpenGL);

        // Create Vertex Buffer.
        objectVertexBuffer = VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, const_cast<float *>(vertices), sizeof(vertices));

        // Set layout for the vertex buffer.
        objectVertexBuffer->SetLayout({
            { Vulkyrie::Renderer::ShaderDataType::Float3, "aPos" },
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

    void PongLayer2::OnAttach() {
        VDEBUG("Layer Attached: Pong Layer 2.");
    }

    void PongLayer2::OnDetach() {
        VDEBUG("Layer Detached: Pong Layer 2.");
    }

    void PongLayer2::OnUpdate(Vulkyrie::Core::Timestep deltaTime) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update Camera position based on input.
        constexpr float cameraSpeed = 5.0f;
        auto dt = deltaTime.GetSeconds();

        if (_application.IsKeyPressed(KeyCode::W)) camera.ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
        if (_application.IsKeyPressed(KeyCode::S)) camera.ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
        if (_application.IsKeyPressed(KeyCode::A)) camera.ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
        if (_application.IsKeyPressed(KeyCode::D)) camera.ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);

        objectShader->Use();
        objectShader->SetVec3Uniform("objectColor", 1.0f, 0.5f, 0.31f);
        objectShader->SetVec3Uniform("lightColor", 1.0f, 1.0f, 1.0f);
        objectShader->SetVec3Uniform("viewPos", camera.GetPosition());

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        objectShader->SetMat4Uniform("projection", projection);
        objectShader->SetMat4Uniform("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        objectShader->SetMat4Uniform("model", model);

        // Set the material properties for the object.
        objectShader->SetVec3Uniform("material.ambient", 1.0f, 0.5f, 0.31f);
        objectShader->SetVec3Uniform("material.diffuse", 1.0f, 0.5f, 0.31f);
        objectShader->SetVec3Uniform("material.specular", 0.5f, 0.5f, 0.5f);
        objectShader->SetFloatUniform("material.shininess", 32.0f);

        // Set the light properties.
        objectShader->SetVec3Uniform("light.position", lightPos);
        objectShader->SetVec3Uniform("light.specular", 1.0f, 1.0f, 1.0f);

        // Change ambient and diffuse light color over time.
        glm::vec3 lightColor;
        double currentTime = glfwGetTime();
        lightColor.x = sin(currentTime * 2.0f);
        lightColor.y = sin(currentTime * 0.7f);
        lightColor.z = sin(currentTime * 1.3f);

        glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);   // decrease the influence
        glm::vec3 ambientColor = diffuseColor * glm::vec3(0.3f); // low influence

        objectShader->SetVec3Uniform("light.ambient", ambientColor);
        objectShader->SetVec3Uniform("light.diffuse", diffuseColor); // darken diffuse light a bit

        // Issue a draw call to draw the light source.
        objectVertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // -----------------------------------------------------------------------------------
        // also draw the lamp object
        lightShader->Use();
        lightShader->SetMat4Uniform("projection", projection);
        lightShader->SetMat4Uniform("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        lightShader->SetMat4Uniform("model", model);

        // graphicsShader.Use();
        lightVertexArray->Bind();

        // Issue a draw call to draw the reflecting object.
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    void PongLayer2::OnEvent(Vulkyrie::Events::Event &event) {
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
    }

} // namespace Pong
