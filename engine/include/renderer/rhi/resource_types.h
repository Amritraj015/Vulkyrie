#pragma once

#include "renderer/rhi/formats.h"
#include "vlkypch.h"
#include "core/types/handle.h"
#include "core/utilities/hash_builder.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    enum class TextureDimensions : u8 { Texture1D, Texture2D, Texture3D, CubeMap };

    enum class TextureUsage : u32 {
        None = 0,
        Sampled = BIT(0),
        Storage = BIT(1),
        RenderTarget = BIT(2),
        DepthStencil = BIT(3),
        CopySrc = BIT(4),
        CopyDst = BIT(5),
        TransientAttach = BIT(6)
    };

    [[nodiscard]] VE_INLINE constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept {
        return static_cast<TextureUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    [[nodiscard]] VE_INLINE constexpr bool HasFlag(TextureUsage a, TextureUsage b) noexcept {
        return 0 != (static_cast<u32>(a) & static_cast<u32>(b));
    }

    enum class BufferUsage : u32 {
        None = 0,
        Vertex = BIT(0),
        Index = BIT(1),
        Uniform = BIT(2),
        Storage = BIT(3),
        Indirect = BIT(4),
        CopySrc = BIT(5),
        CopyDst = BIT(6),
        DeviceAddress = BIT(7),
    };

    [[nodiscard]] VE_INLINE constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) noexcept {
        return static_cast<BufferUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    [[nodiscard]] VE_INLINE constexpr bool HasFlag(BufferUsage a, BufferUsage b) noexcept {
        return 0 != (static_cast<u32>(a) & static_cast<u32>(b));
    }

    enum class MemoryDomain : u8 {
        DeviceLocal,  // VRAM. Everything the GPU reads more than once.
        HostUpload,   // write-combined, CPU-write / GPU-read. Staging + per-frame constants.
        HostReadback, // cached, GPU-write / CPU-read. Query results, screenshots, GPU feedback.
        DeviceUpload, // resizable BAR: VRAM the CPU can write directly. Best for small hot updates.
    };

    struct TextureDescriptor final {
        u32 Width = 1;
        u32 Height = 1;
        u32 Depth = 1;
        u32 Mips = 1;
        u32 Layers = 1;
        Format Format = Format::Undefined;
        TextureDimensions Dimension = TextureDimensions::Texture2D;
        SampleCount Samples = SampleCount::None;
        TextureUsage Usage = TextureUsage::Sampled;

        // StaticString DebugName;

        friend constexpr bool operator==(const TextureDescriptor &, const TextureDescriptor &) = default;
    };

    struct BufferDescriptor final {
        u64 Size = 0;
        BufferUsage Usage = BufferUsage::None;
        MemoryDomain Domain = MemoryDomain::DeviceLocal;

        // StaticString DebugName;

        friend constexpr bool operator==(const BufferDescriptor &, const BufferDescriptor &) = default;
    };

    /** @brief Identifies a texture descriptor registered with the device.
     *
     * Registration is where a descriptor's identity is established - hashed once, deduplicated, sized by the
     * driver - so everything afterwards is an array index. Declaring a transient every frame therefore costs no
     * hashing and no map probing at all; see `TransientRegistry`. */
    using TransientTextureID = Handle<struct TransientTextureTag>;

    /** @brief Identifies a buffer descriptor registered with the device. See `TransientTextureID`. */
    using TransientBufferID = Handle<struct TransientBufferTag>;

    enum class Filter : u8 { Nearest, Linear };
    enum class MipFilter : u8 { Nearest, Linear };
    enum class AddressMode : u8 { Repeat, MirrorRepeat, ClampEdge, ClampBorder };

    struct SamplerDescriptor final {
        f32 MipLodBias = 0.0f;
        f32 MaxAnisotropy = 1.0f;
        f32 MinLod = 0.0f;
        f32 MaxLod = 1000.0f;
        Filter MinFilter = Filter::Linear;
        Filter MagFilter = Filter::Linear;
        MipFilter MipFilter = MipFilter::Linear;
        AddressMode AddressU = AddressMode::Repeat;
        AddressMode AddressV = AddressMode::Repeat;
        AddressMode AddressW = AddressMode::Repeat;
        CompareOp CompareOp = CompareOp::Never;
        bool CompareEnable = false;
    };

    /** @brief The execution-order interval over which a transient resource is live, as positions in the order its
     * producer walks. Two resources whose intervals are disjoint can share one underlying allocation.
     *
     * Shared between the frame graph, which derives the interval while compiling, and `TransientPool`, which uses
     * it to decide whether an existing resource can be handed out again within the same frame. */
    struct ResourceLifetime final {
        u32 FirstUse = 0;
        u32 LastUse = 0;

        [[nodiscard]] constexpr bool Valid() const noexcept {
            return LastUse >= FirstUse;
        }

        /** @brief Whether this interval and `other` never overlap, so one resource can back both. */
        [[nodiscard]] constexpr bool DisjointFrom(ResourceLifetime other) const noexcept {
            return FirstUse > other.LastUse || LastUse < other.FirstUse;
        }

        friend constexpr bool operator==(ResourceLifetime, ResourceLifetime) = default;
    };

    static_assert(std::is_trivially_copyable_v<ResourceLifetime>, "ResourceLifetime must be trivially copyable so it can be passed in registers.");

    /** @brief A CPU-side estimate of the bytes a texture's contents occupy, summed over the mip chain.
     *
     * An estimate, and deliberately labelled as one: it ignores optimal-tiling padding, mip-tail packing and the
     * driver's alignment rounding, and can read 20-50% low. That is fine for the yardstick a
     * `FrameGraphAliasingReport` publishes and fatally wrong for a packer, which would place two resources
     * overlapping when their real extents do not fit. A backend that can actually alias must answer
     * `GetImageMemoryRequirements` from the driver instead of calling this.
     * @param d The descriptor to size. */
    [[nodiscard]] VE_INLINE u64 EstimateTextureBytes(const TextureDescriptor &d) noexcept {
        const u32 bytesPerBlock = BytesPerBlock(d.Format);
        const u32 blockDim = std::max(BlockDim(d.Format), 1U);

        u64 size = 0;

        for (u32 mip = 0; mip < std::max(d.Mips, 1U); ++mip) {
            const u32 width = std::max(d.Width >> mip, 1U);
            const u32 height = std::max(d.Height >> mip, 1U);
            const u32 depth = std::max(d.Depth >> mip, 1U);

            const u64 blocksWide = (width + blockDim - 1) / blockDim;
            const u64 blocksHigh = (height + blockDim - 1) / blockDim;

            size += blocksWide * blocksHigh * depth * bytesPerBlock;
        }

        // SampleCount's enumerators are the sample counts themselves, so the value doubles as the multiplier.
        size *= std::max(d.Layers, 1U);
        size *= static_cast<u64>(d.Samples);

        return size;
    }

    [[nodiscard]] VE_INLINE constexpr u64 HashDescriptor(const TextureDescriptor &d) noexcept {
        HashBuilder hb;

        return hb.Value(d.Width)
            .Value(d.Height)
            .Value(d.Depth)
            .Value(d.Mips)
            .Value(d.Layers)
            .Value(d.Format)
            .Value(d.Dimension)
            .Value(d.Samples)
            .Value(d.Usage)
            .Finish();
    }

    [[nodiscard]] VE_INLINE constexpr u64 HashDescriptor(const BufferDescriptor &d) noexcept {
        HashBuilder hb;

        return hb.Value(d.Size).Value(d.Usage).Value(d.Domain).Finish();
    }

    [[nodiscard]] VE_INLINE constexpr u64 HashDescriptor(const SamplerDescriptor &d) noexcept {
        HashBuilder hb;

        return hb.Value(d.MipLodBias)
            .Value(d.MaxAnisotropy)
            .Value(d.MinLod)
            .Value(d.MaxLod)
            .Value(d.MinFilter)
            .Value(d.MagFilter)
            .Value(d.MipFilter)
            .Value(d.AddressU)
            .Value(d.AddressV)
            .Value(d.AddressW)
            .Value(d.CompareOp)
            .Value(d.CompareEnable)
            .Finish();
    }

} // namespace Vulkyrie
