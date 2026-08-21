#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/frame_graph/frame_graph_context.h"
#include "renderer/rhi/resource_types.h"

namespace Vulkyrie {

    /** @brief A buffer the frame graph owns for the duration of a frame, backed by `TransientPool`. The buffer
     * counterpart of `FrameGraphTexture`; see it for why the verbs are acquire and release.
     *
     * @tparam B The renderer backend the buffer is acquired from. */
    template <RendererBackend B> class FrameGraphBuffer final {
    public:
        using Descriptor = BufferDescriptor;

        FrameGraphBuffer() = default;

        /** @brief Takes a buffer from the frame's transient pool.
         * @param descriptor What the buffer must look like; the pool keys its buckets on this.
         * @param lifetime The execution-order interval this buffer is live over. Two buffers with the same
         * descriptor and disjoint intervals share one allocation.
         * @param placement Where the graph's byte-packing plan put this buffer; see `FrameGraphTexture::Acquire`.
         * @param context The frame context, for reaching the pool. */
        void Acquire(const Descriptor &descriptor, ResourceLifetime lifetime, ResourcePlacement placement, const FrameGraphContext<B> &context) {
            if constexpr (B::kHasMemoryAliasing) {
                // TODO: see FrameGraphTexture::Acquire - suballocating from a heap needs RHI support that does not
                // exist yet.
                VASSERT(!placement.IsAliased, "Byte-offset buffer placement needs RHI heap support that does not exist yet.");
            } else {
                (void)placement;
            }

            mBuffer = context.Device.GetTransients().Acquire(descriptor, lifetime);
        }

        /** @brief Returns the buffer to the pool, clearing the handle so a use-after-release is caught.
         * @param context The frame context, unused. */
        void Release(const FrameGraphContext<B> &context) {
            (void)context;

            mBuffer = typename B::Buffer{};
        }

        /** @brief Reports the storage this buffer needs. Exact, unlike a texture's estimate.
         * @param descriptor The descriptor to size. */
        [[nodiscard]] static ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor) {
            return ResourceMemoryRequirements{ .Size = descriptor.Size, .Alignment = BUFFER_ALIGNMENT };
        }

        /** @brief Returns the underlying backend buffer. Empty until the graph acquires it. */
        [[nodiscard]] VE_INLINE const typename B::Buffer &Buffer() const noexcept {
            return mBuffer;
        }

    private:
        /** @brief Conservative buffer alignment for the sizing estimate; covers the usual storage/uniform limits. */
        static constexpr u64 BUFFER_ALIGNMENT = 256;

        typename B::Buffer mBuffer{};
    };

} // namespace Vulkyrie
