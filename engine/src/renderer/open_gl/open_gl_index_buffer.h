#pragma once

#include "renderer/index_buffer.h"

namespace Vulkyrie {

    /** @brief One entry in the pool: the GL buffer plus what is needed to describe and validate it. */
    struct OpenGLIndexBufferSlot {
    public:
        /** @brief Number of indices, not bytes. */
        size_t Count = 0;

        /** @brief The GL buffer name, or 0 when the slot is free. */
        u32 EBO = 0;

        /** @brief Bumped on every release, so handles to a recycled slot are detectably stale. */
        u32 Generation = 0;

        /** @brief Width of the indices this slot holds. Only meaningful while the slot is live. */
        IndexType Type = IndexType::U32;

        /** @brief Marks the slot free and invalidates every handle pointing at it. Deliberately leaves `Type`
         * alone: a free slot is unreachable, because no handle can pass the generation check afterwards. */
        void Reset() {
            Count = 0;
            EBO = 0;
            ++Generation;
        }
    };

    class OpenGLIndexBuffer final : public IndexBuffer {
    public:
        OpenGLIndexBuffer() = default;

        VE_DELETE_MOVE_AND_COPY(OpenGLIndexBuffer);

        ~OpenGLIndexBuffer() override;

        [[nodiscard]] VE_INLINE std::optional<size_t> GetCount(IndexBufferHandle bufferHandle) const override {
            const OpenGLIndexBufferSlot *slot = resolve(bufferHandle);

            return slot != nullptr ? std::make_optional(slot->Count) : std::nullopt;
        }

        [[nodiscard]] VE_INLINE std::optional<IndexType> GetIndexType(IndexBufferHandle bufferHandle) const override {
            const OpenGLIndexBufferSlot *slot = resolve(bufferHandle);

            return slot != nullptr ? std::make_optional(slot->Type) : std::nullopt;
        }

        /** @brief Gets the GL buffer name behind a handle, for attaching it to a VAO or issuing a draw.
         *
         * OpenGL-specific, so deliberately not on the `IndexBuffer` interface: only code that already knows which
         * backend it is talking to has any use for a raw GL name.
         *
         * @param bufferHandle The handle of the index buffer.
         * @returns The buffer name, or nullopt if the handle is invalid or stale.
         */
        [[nodiscard]] VE_INLINE std::optional<u32> GetBufferID(IndexBufferHandle bufferHandle) const {
            const OpenGLIndexBufferSlot *slot = resolve(bufferHandle);

            return slot != nullptr ? std::make_optional(slot->EBO) : std::nullopt;
        }

        [[nodiscard]] IndexBufferHandle Create(std::span<const u32> indices) override;
        [[nodiscard]] IndexBufferHandle Create(std::span<const u16> indices) override;
        [[nodiscard]] IndexBufferHandle Create(std::span<const u8> indices) override;

        bool Destroy(IndexBufferHandle bufferHandle) override;

    private:
        /** @brief Resolves a handle to its slot, rejecting anything invalid, out of range, or stale. Every public
         * query goes through here so the validation lives in one place rather than once per accessor.
         * @param bufferHandle The handle to resolve.
         * @returns The slot, or nullptr if the handle does not name a live buffer.
         */
        [[nodiscard]] VE_INLINE const OpenGLIndexBufferSlot *resolve(IndexBufferHandle bufferHandle) const {
            if (!bufferHandle.IsValid() || bufferHandle.Index() >= _indexBuffers.size()) {
                return nullptr;
            }

            const OpenGLIndexBufferSlot &slot = _indexBuffers[bufferHandle.Index()];

            return slot.Generation == bufferHandle.Generation() ? &slot : nullptr;
        }

        /** @brief Non-const form of `resolve`, for the one caller that mutates the slot it finds. */
        [[nodiscard]] VE_INLINE OpenGLIndexBufferSlot *resolve(IndexBufferHandle bufferHandle) {
            return const_cast<OpenGLIndexBufferSlot *>(std::as_const(*this).resolve(bufferHandle));
        }

        /** @brief Claims a slot and uploads the index data. The three typed overloads are one-line wrappers over
         * this, so the byte-size arithmetic and the GL calls exist in exactly one place.
         * @param data The index data to upload.
         * @param sizeInBytes Size of `data` in bytes.
         * @param count Number of indices in `data`.
         * @param indexType Width of those indices.
         * @returns A handle to the new buffer, or an invalid handle if there was nothing to upload.
         */
        [[nodiscard]] IndexBufferHandle createBuffer(const void *data, size_t sizeInBytes, size_t count, IndexType indexType);

        /** @brief Every slot ever claimed; released slots stay in place and are recycled through `_freeIndices`. */
        std::vector<OpenGLIndexBufferSlot> _indexBuffers;

        /** @brief Indices of released slots, ready to be reused. */
        std::vector<u32> _freeIndices;
    };

} // namespace Vulkyrie
