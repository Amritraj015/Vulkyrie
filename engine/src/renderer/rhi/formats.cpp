#include "renderer/rhi/formats.h"

#include "core/asserts.h"

namespace Vulkyrie {

    namespace {

        struct FormatInfo final {
            u8 BytesPerBlock;
            u8 BlockDim;
            bool Depth;
            bool Stencil;
            bool Compressed;
        };

        // One row per Format, in declaration order. bytesPerBlock is bytes per texel for
        // uncompressed formats and bytes per 4x4 block for the BC family; blockDim stays 1
        // on uncompressed rows so callers can divide by it unconditionally.
        constexpr FormatInfo kTable[] = {
            { 0, 1, false, false, false }, // Undefined

            { 1, 1, false, false, false }, // R8Unorm
            { 1, 1, false, false, false }, // R8Uint
            { 2, 1, false, false, false }, // RG8Unorm
            { 4, 1, false, false, false }, // RGBA8Unorm
            { 4, 1, false, false, false }, // RGBA8Srgb
            { 4, 1, false, false, false }, // RGBA8Uint
            { 4, 1, false, false, false }, // BGRA8Unorm
            { 4, 1, false, false, false }, // BGRA8Srgb

            { 2, 1, false, false, false }, // R16Uint
            { 2, 1, false, false, false }, // R16Float
            { 4, 1, false, false, false }, // RG16Snorm
            { 4, 1, false, false, false }, // RG16Float
            { 8, 1, false, false, false }, // RGBA16Float

            { 4, 1, false, false, false },  // R32Uint
            { 4, 1, false, false, false },  // R32Float
            { 8, 1, false, false, false },  // RG32Float
            { 12, 1, false, false, false }, // RGB32Float
            { 16, 1, false, false, false }, // RGBA32Float

            { 4, 1, false, false, false }, // R11G11B10Float
            { 4, 1, false, false, false }, // RGB10A2Unorm
            { 4, 1, false, false, false }, // RGB9E5Float

            { 2, 1, true, false, false }, // D16Unorm
            { 4, 1, true, false, false }, // D32Float
            { 4, 1, true, true, false },  // D24UnormS8Uint
            { 8, 1, true, true, false },  // D32FloatS8Uint, two planes; 8 is the padded stride

            { 8, 4, false, false, true },  // BC1Unorm
            { 8, 4, false, false, true },  // BC1Srgb
            { 16, 4, false, false, true }, // BC3Unorm
            { 16, 4, false, false, true }, // BC3Srgb
            { 8, 4, false, false, true },  // BC4Unorm
            { 16, 4, false, false, true }, // BC5Unorm
            { 16, 4, false, false, true }, // BC6HFloat
            { 16, 4, false, false, true }, // BC7Unorm
            { 16, 4, false, false, true }, // BC7Srgb
        };

        static_assert(std::size(kTable) == static_cast<usize>(Format::Count), "kTable needs exactly one row per Format, in declaration order");

        [[nodiscard]] const FormatInfo &Lookup(Format format) noexcept {
            const auto index = static_cast<usize>(format);
            VASSERT(index < std::size(kTable), "Format index {} is out of range", index);
            return kTable[index];
        }

    } // namespace

    [[nodiscard]] bool IsDepthFormat(Format format) noexcept {
        return Lookup(format).Depth;
    }

    [[nodiscard]] bool IsStencilFormat(Format format) noexcept {
        return Lookup(format).Stencil;
    }

    [[nodiscard]] bool IsCompressed(Format format) noexcept {
        return Lookup(format).Compressed;
    }

    [[nodiscard]] u32 BytesPerBlock(Format format) noexcept {
        return Lookup(format).BytesPerBlock;
    }

    [[nodiscard]] u32 BlockDim(Format format) noexcept {
        return Lookup(format).BlockDim;
    }

} // namespace Vulkyrie
