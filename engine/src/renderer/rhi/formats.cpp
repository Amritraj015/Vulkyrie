#include "renderer/rhi/formats.h"

#include "core/asserts.h"

namespace Vulkyrie {

    namespace {

        struct FormatInfo final {
            Format Fmt;
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
            { Format::Undefined,        0, 1, false, false, false },

            { Format::R8Unorm,          1, 1, false, false, false },
            { Format::R8Uint,           1, 1, false, false, false },
            { Format::RG8Unorm,         2, 1, false, false, false },
            { Format::RGBA8Unorm,       4, 1, false, false, false },
            { Format::RGBA8Srgb,        4, 1, false, false, false },
            { Format::RGBA8Uint,        4, 1, false, false, false },
            { Format::BGRA8Unorm,       4, 1, false, false, false },
            { Format::BGRA8Srgb,        4, 1, false, false, false },

            { Format::R16Uint,          2, 1, false, false, false },
            { Format::R16Float,         2, 1, false, false, false },
            { Format::RG16Snorm,        4, 1, false, false, false },
            { Format::RG16Float,        4, 1, false, false, false },
            { Format::RGBA16Float,      8, 1, false, false, false },

            { Format::R32Uint,          4, 1, false, false, false },
            { Format::R32Float,         4, 1, false, false, false },
            { Format::RG32Float,        8, 1, false, false, false },
            { Format::RGB32Float,      12, 1, false, false, false },
            { Format::RGBA32Float,     16, 1, false, false, false },

            { Format::R11G11B10Float,   4, 1, false, false, false },
            { Format::RGB10A2Unorm,     4, 1, false, false, false },
            { Format::RGB9E5Float,      4, 1, false, false, false },

            { Format::D16Unorm,         2, 1, true, false, false },
            { Format::D32Float,         4, 1, true, false, false },
            { Format::D24UnormS8Uint,   4, 1, true, true, false },
            { Format::D32FloatS8Uint,   8, 1, true, true, false },  // two planes; 8 is the padded stride

            { Format::BC1Unorm,         8, 4, false, false, true },
            { Format::BC1Srgb,          8, 4, false, false, true },
            { Format::BC3Unorm,        16, 4, false, false, true },
            { Format::BC3Srgb,         16, 4, false, false, true },
            { Format::BC4Unorm,         8, 4, false, false, true },
            { Format::BC5Unorm,        16, 4, false, false, true },
            { Format::BC6HFloat,       16, 4, false, false, true },
            { Format::BC7Unorm,        16, 4, false, false, true },
            { Format::BC7Srgb,         16, 4, false, false, true },
        };

        static_assert(std::size(kTable) == static_cast<usize>(Format::Count), "kTable needs exactly one row per Format, in declaration order");

        // Row i must carry enum value i, because every accessor indexes by the enum. The count
        // assert above cannot see a format inserted into the MIDDLE of Format with a row appended
        // at the end here: the sizes agree while every format past the insertion point reports
        // its neighbour's size.
        consteval bool RowsMatchEnum() {
            for (usize i = 0; i < std::size(kTable); ++i) {
                if (static_cast<usize>(kTable[i].Fmt) != i) return false;
            }

            return true;
        }

        static_assert(RowsMatchEnum(), "kTable is not in Format declaration order");

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
