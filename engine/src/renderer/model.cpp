#include "core/logger.h"
#include "renderer/open_gl/open_gl_model.h"

namespace Vulkyrie::Renderer {
    Ref<Model> Model::Create(Vulkyrie::Core::GraphicsAPI api, const std::filesystem::path &path, bool gamma) {
        switch (api) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLModel>(path, gamma);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for Model creation!");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
