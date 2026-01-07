#include "pong_layer_3.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Pong {
    using namespace Vulkyrie::Events;

    float vertices[] = {
        // positions         // normals          // texture coords
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

    static constexpr unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    static glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

    PongLayer3::PongLayer3(Vulkyrie::Core::Application &application, f32 windowWidth, f32 windowHeight)
        : Vulkyrie::Core::Layer(application), windowWidth(windowWidth), windowHeight(windowHeight), camera(glm::vec3(0.0f, 0.0f, 5.0f)) {

        // load and compile the shader programs.
        objectShader =
            GraphicsShader::Create(GraphicsAPI::OpenGL, "assets/shaders/specular-highlight.vert.glsl", "assets/shaders/specular-highlight.frag.glsl");
        lightShader = GraphicsShader::Create(GraphicsAPI::OpenGL, "assets/shaders/light-source.vert.glsl", "assets/shaders/light-source.frag.glsl");

        // Check if shaders are loaded successfully.
        if (!objectShader->IsValid() || !lightShader->IsValid()) {
            VERROR("Failed to load shaders.");
            return;
        }

        // load the textures.
        boxTexture = Texture2D::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, "assets/textures/container2.png");
        specularMapTexture = Texture2D::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, "assets/textures/container2_specular.png");

        // Check if textures are loaded successfully.
        if (!boxTexture->IsLoaded() || !specularMapTexture->IsLoaded()) {
            VERROR("Failed to load one or more textures!");
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
            { Vulkyrie::Renderer::ShaderDataType::Float2, "aTexture" },
        });

        // Add Vertex Buffer to the vertex array.
        objectVertexArray->AddVertexBuffer(objectVertexBuffer);

        // Create the vertex array for the light source.
        lightVertexArray = VertexArray::Create(Vulkyrie::Core::GraphicsAPI::OpenGL);

        // Reuse the same vertex buffer for the light source.
        lightVertexArray->AddVertexBuffer(objectVertexBuffer);

        // This is required to make sure 3D rendering works properly.
        glEnable(GL_DEPTH_TEST);
    }

    void PongLayer3::OnAttach() {
        VDEBUG("Layer Attached: Pong Layer 3.");
    }

    void PongLayer3::OnDetach() {
        VDEBUG("Layer Detached: Pong Layer 3.");
    }

    void PongLayer3::OnUpdate(Vulkyrie::Core::Timestep deltaTime) {
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
        objectShader->SetIntUniform("material.diffuse", 0);
        objectShader->SetIntUniform("material.specular", 1);
        objectShader->SetFloatUniform("material.shininess", 32.0f);

        // Bind the textures to texture units.
        boxTexture->Bind(0);
        specularMapTexture->Bind(1);

        // Set the light properties.
        objectShader->SetVec3Uniform("light.position", lightPos);
        objectShader->SetVec3Uniform("light.ambient", 0.2f, 0.2f, 0.2f);
        objectShader->SetVec3Uniform("light.diffuse", 0.5f, 0.5f, 0.5f);
        objectShader->SetVec3Uniform("light.specular", 1.0f, 1.0f, 1.0f);

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

    void PongLayer3::OnEvent(Vulkyrie::Events::Event &event) {
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
