#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    class IndexBuffer {
        public:
            virtual ~IndexBuffer() = default;

            /** @brief Gets the count of indices in the index buffer.
             * @returns The count of indices.
             */
            virtual size_t GetCount() const = 0;

            /** @brief Creates an index buffer with given unsigned 32-bit indices and count.
             * @param indices Pointer to unsigned 32-bit index data.
             * @param count Number of indices.
             * @returns A reference to the created IndexBuffer.
             */
            static Ref<IndexBuffer> Create(u32 *indices, size_t count);

            /** @brief Creates an index buffer with given unsigned 16-bit indices and count.
             * @param indices Pointer to unsigned 16-bit index data.
             * @param count Number of indices.
             * @returns A reference to the created IndexBuffer.
             */
            static Ref<IndexBuffer> Create(u16 *indices, size_t count);

            /** @brief Creates an index buffer with given unsigned 8-bit indices and count.
             * @param indices Pointer to unsigned 8-bit index data.
             * @param count Number of indices.
             * @returns A reference to the created IndexBuffer.
             */
            static Ref<IndexBuffer> Create(u8 *indices, size_t count);
    };

} // namespace Vulkyrie
