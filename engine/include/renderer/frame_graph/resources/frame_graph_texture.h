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
        /** @brief What the graph is handed to declare one of these: a registered descriptor's id, not the
         * descriptor itself.
         *
         * The id is the whole point. Sizing it for the aliasing plan and finding its pool bucket are both array
         * indices, so declaring a transient every frame costs no hashing at all. Resolve it back to the
         * `TextureDescriptor` through `Device::GetRegistry().Descriptor(id)` when a pass needs the extent. */
        using Descriptor = TransientTextureID;

        FrameGraphTexture() = default;

        /** @brief Takes an image from the frame's transient pool.
         * @param descriptor The registered descriptor; the pool keys its buckets on its id.
         * @param lifetime The execution-order interval this texture is live over. Two textures with the same
         * descriptor and disjoint intervals share one image.
         * @param placement Where the graph's byte-packing plan put this texture. Only meaningful on a backend that
         * can bind two resources to one allocation; elsewhere the graph hands over an unplaced placement.
         * @param context The frame context, for reaching the pool. */
        void Acquire(const Descriptor &descriptor, ResourceLifetime lifetime, ResourcePlacement placement, const FrameGraphContext<B> &context) {
            if constexpr (B::kHasMemoryAliasing) {
                // TODO: bind into heap `placement.BlockIndex` at `placement.Offset` once the RHI grows heaps
                // (HeapDescriptor in rhi/memory_types.h is still commented out). Until then the pool's
                // whole-resource reuse is the only aliasing in play, and honouring an offset is not expressible.
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

        /** @brief Reports the storage this texture needs, as the driver reports it.
         *
         * An array read: the backend was asked once, when the descriptor was registered. Sizing a texture by
         * summing its mip chain ignores optimal-tiling padding, mip-tail packing and alignment rounding, and reads
         * 20-50% low on real hardware - which for a report is merely imprecise and for a packer is unsound,
         * because two resources then get placed overlapping when their real extents do not fit.
         * @param descriptor The registered descriptor to size.
         * @param device The device holding the registry that has the answer. */
        [[nodiscard]] static ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor, const Device<B> &device) {
            return device.GetRegistry().Requirements(descriptor);
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
        typename B::Image mImage{};

        bool mRequiresDiscard = true;
    };

} // namespace Vulkyrie
