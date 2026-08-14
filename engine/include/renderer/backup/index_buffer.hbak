#pragma once

#include "vlkypch.h"
#include "renderer/renderer_types.h"

namespace Vulkyrie {

    /** @brief Width of the indices held by an index buffer.
     *
     * Backend-agnostic on purpose: a draw call has to know the index width, and each backend maps this to its own
     * spelling (`GL_UNSIGNED_SHORT`, `VK_INDEX_TYPE_UINT16`, ...). Without it a handle is not enough to draw with,
     * because nothing can recover which of the typed `Create` overloads produced it. */
    enum class IndexType : u8 { U8, U16, U32 };

    /** @brief A pool of index buffers addressed by generational handle rather than by object identity.
     *
     * One instance owns many buffers: `Create` claims a slot and returns a handle, `Destroy` releases the slot for
     * reuse and bumps its generation. Every query rejects a handle whose generation no longer matches, so a stale
     * handle reads as "gone" instead of silently aliasing whatever buffer took the slot next. */
    class IndexBuffer {
    public:
        VE_DELETE_MOVE_AND_COPY(IndexBuffer);

        virtual ~IndexBuffer() = default;

        /** @brief Gets the number of indices in a buffer.
         * @param bufferHandle The handle of the index buffer.
         * @returns The index count, or nullopt if the handle is invalid or stale.
         */
        [[nodiscard]] virtual std::optional<size_t> GetCount(IndexBufferHandle bufferHandle) const = 0;

        /** @brief Gets the width of the indices in a buffer, which a draw call needs in order to interpret them.
         * @param bufferHandle The handle of the index buffer.
         * @returns The index type, or nullopt if the handle is invalid or stale.
         */
        [[nodiscard]] virtual std::optional<IndexType> GetIndexType(IndexBufferHandle bufferHandle) const = 0;

        /** @brief Creates an index buffer from unsigned 32-bit indices.
         * @param indices The index data; copied into the buffer, so the span need not outlive the call.
         * @returns A handle to the new buffer, or an invalid handle if `indices` is empty.
         */
        [[nodiscard]] virtual IndexBufferHandle Create(std::span<const u32> indices) = 0;

        /** @brief Creates an index buffer from unsigned 16-bit indices.
         * @param indices The index data; copied into the buffer, so the span need not outlive the call.
         * @returns A handle to the new buffer, or an invalid handle if `indices` is empty.
         */
        [[nodiscard]] virtual IndexBufferHandle Create(std::span<const u16> indices) = 0;

        /** @brief Creates an index buffer from unsigned 8-bit indices.
         * @param indices The index data; copied into the buffer, so the span need not outlive the call.
         * @returns A handle to the new buffer, or an invalid handle if `indices` is empty.
         */
        [[nodiscard]] virtual IndexBufferHandle Create(std::span<const u8> indices) = 0;

        /** @brief Destroys an index buffer, releasing its slot for reuse and invalidating outstanding handles.
         * @param bufferHandle The handle of the index buffer to destroy.
         * @returns True if the buffer was destroyed; false if the handle was invalid or already stale.
         */
        virtual bool Destroy(IndexBufferHandle bufferHandle) = 0;

    protected:
        /** @brief Only a backend constructs one.
         *
         * Declared explicitly because deleting the copy and move operations above counts as user-declaring a
         * constructor, which suppresses the implicit default one and would leave every backend unconstructible. */
        IndexBuffer() = default;
    };

} // namespace Vulkyrie
