#include "core/logger.h"
#include "renderer/graphics_shader.h"
#include "renderer/open_gl/open_gl_graphics_shader.h"

namespace Vulkyrie::Renderer {
    Ref<GraphicsShader> GraphicsShader::Create(GraphicsAPI api, const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath) {
        switch (api) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLGraphicsShader>(vertexShaderPath, fragmentShaderPath);
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for GraphicsShader creation!");
                break;
        }

        return nullptr;
    }
} // namespace Vulkyrie::Renderer
