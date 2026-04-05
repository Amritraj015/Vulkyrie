#include "vlkypch.h"
#include "renderer/shader.h"
#include "renderer/open_gl/open_gl_shader.h"
#include "renderer/renderer.h"

namespace Vulkyrie {

    Ref<Shader> Shader::Create(const std::filesystem::path &shaderSourcePath) {
        switch (GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLShader>(shaderSourcePath);
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for Shader creation!");
                break;
        }

        return nullptr;
    }

} // namespace Vulkyrie
