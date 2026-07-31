#include "vlkypch.h"
#include "renderer/open_gl/open_gl_model.h"
#include "renderer/renderer_context.h"

namespace Vulkyrie {

    Ref<Model> Model::Create(const std::filesystem::path &path, bool gamma) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLModel>(path, gamma);
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for Model creation!");
                return nullptr;
        }
    }

} // namespace Vulkyrie
