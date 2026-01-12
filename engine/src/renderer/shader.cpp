#include "core/logger.h"
#include "renderer/shader.h"
#include "renderer/open_gl/open_gl_shader.h"

namespace Vulkyrie::Renderer {
    Ref<Shader> Shader::Create(GraphicsAPI api, const std::filesystem::path &shaderSourcePath) {
        switch (api) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLShader>(shaderSourcePath);
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for Shader creation!");
                break;
        }

        return nullptr;
    }
} // namespace Vulkyrie::Renderer
