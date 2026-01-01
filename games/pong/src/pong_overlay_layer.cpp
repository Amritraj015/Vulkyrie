#include "pong_overlay_layer.h"

namespace Pong {
    using namespace Vulkyrie::Renderer;
    using namespace Vulkyrie::Core;

    PongOverlayLayer::PongOverlayLayer(const std::string_view layerName) : Layer(layerName) {

        // Load shaders.
        const GraphicsShader graphicsShader("assets/shaders/triangle.vert.glsl", "assets/shaders/triangle.frag.glsl");

        // Check if shader program creation failed.
        if (!graphicsShader.IsValid()) {
            // Log a fatal error.
            VFATAL("Failed to create graphics shader");
        }

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

        _vertexArray = VertexArray::Create(GraphicsAPI::OpenGL);

        const auto vertexBuffer = VertexBuffer::Create(GraphicsAPI::OpenGL, const_cast<float *>(vertices), sizeof(vertices));
        const BufferLayout bufferLayout{
            { ShaderDataType::Float3, "aPos" },
            { ShaderDataType::Float2, "aTexCoord" },
        };
        vertexBuffer->SetLayout(bufferLayout);

        _vertexArray->AddVertexBuffer(vertexBuffer);

        const auto indexBuffer = IndexBuffer::Create(GraphicsAPI::OpenGL, const_cast<u32 *>(indices), sizeof(indices) / sizeof(u32));
        _vertexArray->SetIndexBuffer(indexBuffer);

        // -----------------------------------------------
        // TODO: Add texture loading and binding here.
        // TODO: Add texture loading and binding here.
        // TODO: Add texture loading and binding here.
        // TODO: Add texture loading and binding here.
        // -----------------------------------------------

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
    }

    void PongOverlayLayer::OnAttach() {
        VDEBUG("Layer Attached: {}", _layerName.data());
    }

    void PongOverlayLayer::OnDetach() {
        VDEBUG("Layer Detached: {}", _layerName.data());
    }

    void PongOverlayLayer::OnUpdate(Vulkyrie::Core::Timestep deltaTime) {
    }

    void PongOverlayLayer::OnEvent(Vulkyrie::Events::Event &event) {
        // VINFO("{} - Event: {}",  _layerName.c_str(), event.ToString());
        Vulkyrie::Events::EventDispatcher dispatcher(event);

        dispatcher.Dispatch<Vulkyrie::Events::KeyPressedEvent>([this](const Vulkyrie::Events::KeyPressedEvent &e) {
            if (e.GetKeyCode() == Vulkyrie::Events::KeyCode::J) {
                _toggleWireframe = !_toggleWireframe;

                VINFO("J key pressed in {}!", _layerName.data());
                // Vulkyrie::Core::ApplicationManager::ToggleWireframeMode(_toggleWireframe);

                return true;
            }

            return false;
        });
    }

} // namespace Pong
