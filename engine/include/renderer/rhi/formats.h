#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /**
     * @brief Backend-agnostic texel formats the RHI can express.
     *
     * Grouped by texel size. The numeric values are runtime-only — pipeline hashes
     * built from them never outlive the process — so entries may be reordered, as
     * long as the table in rhi/formats.cpp is reordered to match.
     */
    enum class Format : u16 {
        Undefined = 0,

        // 8-bit per channel
        R8Unorm,
        R8Uint,
        RG8Unorm,
        RGBA8Unorm,
        RGBA8Srgb,
        RGBA8Uint,
        BGRA8Unorm,
        BGRA8Srgb,

        // 16-bit per channel
        R16Uint,
        R16Float,
        RG16Snorm, // octahedral-encoded normals
        RG16Float,
        RGBA16Float,

        // 32-bit per channel
        R32Uint,
        R32Float,
        RG32Float,
        RGB32Float,
        RGBA32Float,

        // Packed
        R11G11B10Float,
        RGB10A2Unorm,
        RGB9E5Float,

        // Depth / stencil. D24UnormS8Uint is the ubiquitous GL depth-stencil format and
        // the cheaper of the two on NVIDIA; D32FloatS8Uint is its AMD-side counterpart.
        D16Unorm,
        D32Float,
        D24UnormS8Uint,
        D32FloatS8Uint,

        // Block compressed, all 4x4 blocks
        BC1Unorm,
        BC1Srgb,
        BC3Unorm,
        BC3Srgb,
        BC4Unorm,
        BC5Unorm,
        BC6HFloat,
        BC7Unorm,
        BC7Srgb,

        Count
    };

    enum class SampleCount : u8 {
        X1 = BIT(0),
        X2 = BIT(1),
        X4 = BIT(2),
        X8 = BIT(3),
    };

    /**
     * @brief Tests whether a format carries a depth aspect.
     * @param format Format to query.
     * @returns True for the D* formats, false otherwise.
     */
    [[nodiscard]] bool IsDepthFormat(Format format) noexcept;

    /**
     * @brief Tests whether a format carries a stencil aspect.
     * @param format Format to query.
     * @returns True for the combined depth-stencil formats, false otherwise.
     */
    [[nodiscard]] bool IsStencilFormat(Format format) noexcept;

    /**
     * @brief Tests whether a format stores texels in compressed blocks.
     * @param format Format to query.
     * @returns True for the BC family, false otherwise.
     */
    [[nodiscard]] bool IsCompressed(Format format) noexcept;

    /**
     * @brief Size of one addressable unit of the format.
     * @param format Format to query.
     * @returns Bytes per texel for uncompressed formats, bytes per 4x4 block for
     *          compressed ones, and 0 for Format::Undefined.
     */
    [[nodiscard]] u32 BytesPerBlock(Format format) noexcept;

    /**
     * @brief Edge length in texels of one compression block.
     * @param format Format to query.
     * @returns 4 for the BC family, 1 for every uncompressed format, so callers can
     *          divide extents by it unconditionally.
     */
    [[nodiscard]] u32 BlockDim(Format format) noexcept;

} // namespace Vulkyrie
