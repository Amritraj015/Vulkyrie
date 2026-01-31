#pragma once

#include "renderer/texture.h"
#include "core/graphics_api.h"

namespace Vulkyrie::Renderer {
    class Texture2D : public Texture {
        public:
            static Ref<Texture2D> Create(const TextureSpecification &specification);
            static Ref<Texture2D> Create(const std::filesystem::path &path);
            static Ref<Texture2D> CreateCubeMap(const std::vector<std::filesystem::path> &paths);
    };
} // namespace Vulkyrie::Renderer
