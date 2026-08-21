#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/frame_graph/frame_graph_context.h"
#include "renderer/rhi/formats.h"
#include "renderer/rhi/resource_types.h"

namespace Vulkyrie {

    /** @brief A texture the frame graph owns for the duration of a frame, backed by `TransientPool`.
     *
     * Acquire and release rather than create and destroy: the pool hands out `B::Image` objects it already has and
     * reclaims them all at frame end, so nothing here builds or tears down a GPU resource per frame. A texture
     * whose lifetime is disjoint from another same-descriptor texture's is handed the same underlying image, which
     * is the reuse the graph's compiled lifetimes exist to enable.
     *
     * @tparam B The renderer backend the image is acquired from. */
    template <RendererBackend B> class FrameGraphTexture final {
    public:
        using Descriptor = TextureDescriptor;

        FrameGraphTexture() = default;

        /** @brief Takes an image from the frame's transient pool.
         * @param descriptor What the image must look like; the pool keys its buckets on this.
         * @param lifetime The execution-order interval this texture is live over. Two textures with the same
         * descriptor and disjoint intervals share one image.
         * @param placement Where the graph's byte-packing plan put this texture. Only meaningful on a backend that
         * can bind two resources to one allocation; elsewhere the graph hands over an unplaced placement.
         * @param context The frame context, for reaching the pool. */
        void Acquire(const Descriptor &descriptor, ResourceLifetime lifetime, ResourcePlacement placement, const FrameGraphContext<B> &context) {
            if constexpr (B::kHasMemoryAliasing) {
                // TODO: bind at placement.Offset once the RHI grows heaps (HeapDescriptor in rhi/memory_types.h is
                // still commented out). Until then the pool's whole-resource reuse is the only aliasing in play,
                // and honouring an offset is not expressible.
                VASSERT(!placement.IsAliased, "Byte-offset texture placement needs RHI heap support that does not exist yet.");
            } else {
                (void)placement;
            }

            const auto acquisition = context.Device.GetTransients().Acquire(descriptor, lifetime);

            mImage = acquisition.Image;
            mRequiresDiscard = acquisition.RequiresDiscard;
        }

        /** @brief Returns the image to the pool.
         *
         * The pool reclaims in bulk at `ResetFrame`, so there is nothing to hand back here. Clearing the handle is
         * the point: it turns a pass that binds a released texture into a Debug assertion instead of a stale bind
         * that renders into whatever took the image over.
         * @param context The frame context, unused. */
        void Release(const FrameGraphContext<B> &context) {
            (void)context;

            mImage = typename B::Image{};
            mRequiresDiscard = true;
        }

        /** @brief Reports the storage this texture would need, so the graph's aliasing plan can measure what
         * packing would save. An estimate over the mip chain, not an allocator's answer - the report it feeds is a
         * yardstick, and on a backend that can actually place resources the driver's requirements win.
         * @param descriptor The descriptor to size. */
        [[nodiscard]] static ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor) {
            // TODO: replace with vkGetImageMemoryRequirements on a throwaway VkImage created
            // from `descriptor`. The estimate below ignores optimal tiling padding, mip-tail
            // packing and alignment rounding, and can be 20-50% low -- which for a PACKER is
            // not merely inaccurate but unsound, because two resources could then be placed
            // overlapping when their real sizes do not fit.
            const u32 bytesPerBlock = BytesPerBlock(descriptor.Format);
            const u32 blockDim = std::max(BlockDim(descriptor.Format), 1U);

            u64 size = 0;

            for (u32 mip = 0; mip < std::max(descriptor.Mips, 1U); ++mip) {
                const u32 width = std::max(descriptor.Width >> mip, 1U);
                const u32 height = std::max(descriptor.Height >> mip, 1U);
                const u32 depth = std::max(descriptor.Depth >> mip, 1U);

                const u64 blocksWide = (width + blockDim - 1) / blockDim;
                const u64 blocksHigh = (height + blockDim - 1) / blockDim;

                size += blocksWide * blocksHigh * depth * bytesPerBlock;
            }

            // SampleCount's enumerators are the sample counts themselves, so the value doubles as the multiplier.
            size *= std::max(descriptor.Layers, 1U);
            size *= static_cast<u64>(descriptor.Samples);

            return ResourceMemoryRequirements{ .Size = size, .Alignment = IMAGE_ALIGNMENT };
        }

        /** @brief Returns the underlying backend image. Empty until the graph acquires it. */
        [[nodiscard]] VE_INLINE const typename B::Image &Image() const noexcept {
            return mImage;
        }

        /** @brief Whether the contents must be treated as undefined - the image is either brand new or was last
         * used by something else. A pass that fully overwrites the texture can ignore this; one that blends into
         * it must not. */
        [[nodiscard]] VE_INLINE bool RequiresDiscard() const noexcept {
            return mRequiresDiscard;
        }

    private:
        /** @brief Conservative image alignment for the sizing estimate. */
        static constexpr u64 IMAGE_ALIGNMENT = 256;

        typename B::Image mImage{};

        bool mRequiresDiscard = true;
    };

} // namespace Vulkyrie
