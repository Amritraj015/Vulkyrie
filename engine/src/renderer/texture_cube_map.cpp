#include "renderer/open_gl/open_gl_texture_cube_map.h"
#include "core/logger.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {
    Ref<TextureCubeMap> TextureCubeMap::Create(std::array<std::filesystem::path, 6> faces) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLTextureCubeMap>(std::move(faces));
            default:
                VERROR("Unsupported Graphics API for TextureCubeMap creation!");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
