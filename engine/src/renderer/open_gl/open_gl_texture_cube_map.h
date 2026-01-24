#pragma once

#include "renderer/texture_cube_map.h"

namespace Vulkyrie::Renderer {
    /** @brief OpenGL implementation of a cube map texture. */
    class OpenGLTextureCubeMap final : public TextureCubeMap {
        public:
            /** @brief Constructs an OpenGL cube map texture from the specified file paths.
             * @param faces The file paths for the cube map faces.
             */
            OpenGLTextureCubeMap(const std::vector<std::filesystem::path> &faces);

            /** @brief Destructor for the OpenGLTextureCubeMap class. */
            ~OpenGLTextureCubeMap();

            /** @brief Binds the cube map texture to the specified slot.
             * @param slot The texture slot to bind to. Default is 0.
             */
            void Bind(u32 slot = 0) const override;
    };
} // namespace Vulkyrie::Renderer
