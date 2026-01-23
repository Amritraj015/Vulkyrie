#pragma once

#include "core/graphics_api.h"

namespace Vulkyrie::Renderer {
    class TextureCubeMap {
        public:
            /** @brief Creates a cube map texture from the specified file paths.
             * @param api The graphics API to use.
             * @param faces The file paths for the cube map faces.
             * @return A reference to the created TextureCubeMap.
             */
            static Ref<TextureCubeMap> Create(Vulkyrie::Core::GraphicsAPI api, const std::vector<std::filesystem::path> &faces);

            /** @brief Gets the renderer-specific texture ID.
             * @returns The texture ID.
             */
            [[nodiscard]] inline u32 GetTextureID() const {
                return _textureId;
            }

            virtual void Bind(u32 slot = 0) const = 0;

        protected:
            /** @brief The renderer-specific texture ID. */
            u32 _textureId;
    };
} // namespace Vulkyrie::Renderer
