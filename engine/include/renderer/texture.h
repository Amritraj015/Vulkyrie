#pragma once

#include "defines.h"

namespace Vulkyrie::Renderer {
    class Texture {
        public:
            Texture(u32 height, u32 width);
            virtual ~Texture() = default;

            /** @brief Gets the width of the texture in pixels.
             * @returns The width of the texture in pixels.
             */
            virtual u32 GetWidth() const = 0;

            /** @brief Gets the height of the texture in pixels.
             * @returns The height of the texture in pixels.
             */
            virtual u32 GetHeight() const = 0;

        private:
            /** @brief Height of the texture in pixels. */
            u32 _height;

            /** @brief Width of the texture in pixels. */
            u32 _width;
    };
} // namespace Vulkyrie::Renderer
