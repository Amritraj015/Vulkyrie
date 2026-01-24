#pragma once

#include "renderer/texture.h"
#include "core/graphics_api.h"

namespace Vulkyrie::Renderer {
    class Texture2D : public Texture {
        public:
            static Ref<Texture2D> Create(Vulkyrie::Core::GraphicsAPI api, const TextureSpecification &specification);
            static Ref<Texture2D> Create(Vulkyrie::Core::GraphicsAPI api, const std::filesystem::path &path);

            static Ref<Texture2D> CreateCubeMap(Vulkyrie::Core::GraphicsAPI api, const std::vector<std::filesystem::path> &paths);
    };
} // namespace Vulkyrie::Renderer
