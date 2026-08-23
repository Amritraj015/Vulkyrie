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
        /** @brief A registered descriptor's id rather than the descriptor; see `FrameGraphTexture::Descriptor`. */
        using Descriptor = TransientBufferID;

        FrameGraphBuffer() = default;

        /** @brief Takes a buffer from the frame's transient pool.
         * @param descriptor The registered descriptor; the pool keys its buckets on its id.
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

            const auto acquisition = context.Device.GetTransients().Acquire(descriptor, lifetime);

            mBuffer = acquisition.Buffer;
            mRequiresDiscard = acquisition.RequiresDiscard;
        }

        /** @brief Returns the buffer to the pool, clearing the handle so a use-after-release is caught.
         * @param context The frame context, unused. */
        void Release(const FrameGraphContext<B> &context) {
            (void)context;

            mBuffer = typename B::Buffer{};
            mRequiresDiscard = true;
        }

        /** @brief Reports the storage this buffer needs, as the driver reports it.
         *
         * An array read: the backend was asked once, when the descriptor was registered. A buffer's size is its
         * descriptor's size, but its alignment and the memory types that can back it are the allocator's answer,
         * not a constant this type can assume.
         * @param descriptor The registered descriptor to size.
         * @param device The device holding the registry that has the answer. */
        [[nodiscard]] static ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor, const Device<B> &device) {
            return device.GetRegistry().Requirements(descriptor);
        }

        /** @brief Returns the underlying backend buffer. Empty until the graph acquires it. */
        [[nodiscard]] VE_INLINE const typename B::Buffer &Buffer() const noexcept {
            return mBuffer;
        }

        /** @brief Whether the contents must be treated as undefined - the buffer is either brand new or was last
         * used by something else. A pass that fully overwrites it can ignore this; one that accumulates into it
         * must not. */
        [[nodiscard]] VE_INLINE bool RequiresDiscard() const noexcept {
            return mRequiresDiscard;
        }

    private:
        typename B::Buffer mBuffer{};

        bool mRequiresDiscard = true;
    };

} // namespace Vulkyrie
