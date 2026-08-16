#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    // enum class ResourceAccess : u32 {
    //     ShaderRead,  // SRV / sampled or storage-read
    //     ShaderWrite, // UAV / storage-write
    //     ColorAttachment,
    //     TransferSrc,
    //     TransferDst,
    //     AccelStructRead,
    //     AccelStructWrite,
    // };

    enum class ResourceState : u16 {
        Undefined = 0,

        // --- read states ---
        VertexBuffer,
        IndexBuffer,
        IndirectArgument,
        UniformBuffer,
        ShaderResourceGraphics,
        ShaderResourceCompute,
        DepthRead,
        CopySrc,
        ResolveSrc,
        Present,

        // --- write states (must remain contiguous and last) ---
        RenderTarget,
        DepthWrite,
        UnorderedAccessGraphics,
        UnorderedAccessCompute,
        CopyDst,
        ResolveDst,

        Count
    };

    [[nodiscard]] VE_INLINE constexpr bool IsWriteState(ResourceState s) noexcept {
        return s >= ResourceState::RenderTarget && s < ResourceState::Count;
    }

    [[nodiscard]] VE_INLINE constexpr bool IsReadState(ResourceState s) noexcept {
        return s > ResourceState::Undefined && s < ResourceState::RenderTarget;
    }

} // namespace Vulkyrie
