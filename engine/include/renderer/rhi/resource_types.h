#pragma once

#include "core/utilities/hash_builder.h"
#include "renderer/rhi/rhi_types.h"
#include "vlkypch.h"

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

    [[nodiscard]] constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept {
        return static_cast<TextureUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    [[nodiscard]] constexpr bool HasFlag(TextureUsage a, TextureUsage b) noexcept {
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

    [[nodiscard]] constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) noexcept {
        return static_cast<BufferUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    [[nodiscard]] constexpr bool HasFlag(BufferUsage a, BufferUsage b) noexcept {
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
        SampleCount Samples = SampleCount::X1;
        TextureUsage Usage = TextureUsage::Sampled;

        // StaticString DebugName;
    };

    struct BufferDescriptor final {
        u64 Size = 0;
        BufferUsage Usage = BufferUsage::None;
        MemoryDomain Domain = MemoryDomain::DeviceLocal;

        // StaticString DebugName;
    };

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

    [[nodiscard]] constexpr u64 HashDescriptor(const TextureDescriptor &d) noexcept {
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

    [[nodiscard]] constexpr u64 HashDescriptor(const BufferDescriptor &d) noexcept {
        HashBuilder hb;

        return hb.Value(d.Size).Value(d.Usage).Value(d.Domain).Finish();
    }

    [[nodiscard]] constexpr u64 HashDescriptor(const SamplerDescriptor &d) noexcept {
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
