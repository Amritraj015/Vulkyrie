#include "renderer/open_gl/open_gl_texture_cube_map.h"
#include "core/logger.h"
#include "core/asserts.h"

namespace Vulkyrie::Renderer {
    Ref<TextureCubeMap> TextureCubeMap::Create(Vulkyrie::Core::GraphicsAPI api, const std::vector<std::filesystem::path> &faces) {

        VASSERT_EXPR(faces.size() == 6, "TextureCubeMap requires exactly 6 faces!");

        switch (api) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLTextureCubeMap>(faces);
            default:
                VERROR("Unsupported Graphics API for TextureCubeMap creation!");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
