#pragma once

#include "vlkypch.h"
#include "renderer/texture_2D.h"

namespace Vulkyrie {
    /** @brief Structure to hold different types of textures associated with a mesh. */
    struct MeshTextures {
        public:
            /** @brief Vector to hold ambient textures. */
            std::vector<Ref<Texture2D>> Ambient;

            /** @brief Vector to hold diffuse textures. */
            std::vector<Ref<Texture2D>> Diffuse;

            /** @brief Vector to hold specular textures. */
            std::vector<Ref<Texture2D>> Specular;

            /** @brief Vector to hold normal textures. */
            std::vector<Ref<Texture2D>> Normal;

            /** @brief Vector to hold height textures. */
            std::vector<Ref<Texture2D>> Height;

            /** @brief Calculates the total number of textures across all types.
             * @return The total count of textures.
             */
            size_t TotalCount() const {
                return Ambient.size() + Diffuse.size() + Specular.size() + Normal.size() + Height.size();
            }

            /** @brief Clears all texture vectors. */
            void Clear() {
                Ambient.clear();
                Diffuse.clear();
                Specular.clear();
                Normal.clear();
                Height.clear();
            }
    };
} // namespace Vulkyrie
