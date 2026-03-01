#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    class IndexBuffer {
        public:
            virtual ~IndexBuffer() = default;

            /** @brief Gets the count of indices in the index buffer.
             * @returns The count of indices.
             */
            virtual u32 GetCount() const = 0;

            /** @brief Creates an index buffer with the given indices and count.
             * @param indices Pointer to the index data.
             * @param count Number of indices.
             * @returns A reference to the created IndexBuffer.
             */
            static Ref<IndexBuffer> Create(u32 *indices, u32 count);
    };

} // namespace Vulkyrie::Renderer
