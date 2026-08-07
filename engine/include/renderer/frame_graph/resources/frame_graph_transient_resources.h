#pragma once

#include "vlkypch.h"
#include "renderer/texture_2D.h"

namespace Vulkyrie {
    class FrameGraphTransientResources {
    public:
        void CreateTexture(const TextureSpecification &specification);
        void DestroyTexture(const TextureSpecification &specification);

        // void CreateBuffer(size_t size);
        // void DestroyBuffer();

    private:
        std::unordered_map<std::string, Scope<Texture2D>> _textureMap;
    };
} // namespace Vulkyrie
