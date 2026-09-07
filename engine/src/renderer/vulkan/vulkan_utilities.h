#pragma once

#include <volk.h>

#include "core/asserts.h"
#include "renderer/rhi/formats.h"

namespace Vulkyrie {

#define VE_VK_CHECK(expr, statusCode)                                                                                                                          \
    do {                                                                                                                                                       \
        const auto result = (expr);                                                                                                                            \
        if (VK_SUCCESS != result) {                                                                                                                            \
            VERROR("Vulkan Error Code: {}", std::to_underlying(result));                                                                                       \
            return statusCode;                                                                                                                                 \
        }                                                                                                                                                      \
    } while (false)

#define VE_VK_TRY_CREATE(expr)                                                                                                                                 \
    do {                                                                                                                                                       \
        const auto result = (expr);                                                                                                                            \
        if (VK_SUCCESS != result) {                                                                                                                            \
            return std::nullopt;                                                                                                                               \
        }                                                                                                                                                      \
    } while (false)

#define VE_VK_EXPECT(expr, statusCode)                                                                                                                         \
    do {                                                                                                                                                       \
        const auto result = (expr);                                                                                                                            \
        if (VK_SUCCESS != result) {                                                                                                                            \
            return std::unexpected(statusCode);                                                                                                                \
        }                                                                                                                                                      \
    } while (false)

    namespace detail {

        struct FormatPair final {
            Format rhi;
            VkFormat vk;
        };

        constexpr usize kFormatCount = static_cast<usize>(Format::Count);

        // One row per Format, in declaration order. Each row names its Format rather than
        // relying on position: the count assert alone cannot see a format inserted into the
        // MIDDLE of the enum with a row appended at the end here, because the sizes still
        // agree while every format past the insertion point maps to its neighbour.
        constexpr FormatPair kFormats[] = {
            { Format::Undefined, VK_FORMAT_UNDEFINED },

            { Format::R8Unorm, VK_FORMAT_R8_UNORM },
            { Format::R8Uint, VK_FORMAT_R8_UINT },
            { Format::RG8Unorm, VK_FORMAT_R8G8_UNORM },
            { Format::RGBA8Unorm, VK_FORMAT_R8G8B8A8_UNORM },
            { Format::RGBA8Srgb, VK_FORMAT_R8G8B8A8_SRGB },
            { Format::RGBA8Uint, VK_FORMAT_R8G8B8A8_UINT },
            { Format::BGRA8Unorm, VK_FORMAT_B8G8R8A8_UNORM },
            { Format::BGRA8Srgb, VK_FORMAT_B8G8R8A8_SRGB },

            { Format::R16Uint, VK_FORMAT_R16_UINT },
            { Format::R16Float, VK_FORMAT_R16_SFLOAT },
            { Format::RG16Snorm, VK_FORMAT_R16G16_SNORM },
            { Format::RG16Float, VK_FORMAT_R16G16_SFLOAT },
            { Format::RGBA16Float, VK_FORMAT_R16G16B16A16_SFLOAT },

            { Format::R32Uint, VK_FORMAT_R32_UINT },
            { Format::R32Float, VK_FORMAT_R32_SFLOAT },
            { Format::RG32Float, VK_FORMAT_R32G32_SFLOAT },
            { Format::RGB32Float, VK_FORMAT_R32G32B32_SFLOAT },
            { Format::RGBA32Float, VK_FORMAT_R32G32B32A32_SFLOAT },

            // B10G11R11, and the components run in that order in memory: the name follows
            // DXGI's R11G11B10, which lists them high bits first.
            { Format::R11G11B10Float, VK_FORMAT_B10G11R11_UFLOAT_PACK32 },
            // A2B10G10R10, not A2R10G10B10: this is the component order DXGI calls
            // R10G10B10A2 and the one every packing helper in the engine assumes.
            { Format::RGB10A2Unorm, VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
            { Format::RGB9E5Float, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 },

            { Format::D16Unorm, VK_FORMAT_D16_UNORM },
            { Format::D32Float, VK_FORMAT_D32_SFLOAT },
            { Format::D24UnormS8Uint, VK_FORMAT_D24_UNORM_S8_UINT },
            { Format::D32FloatS8Uint, VK_FORMAT_D32_SFLOAT_S8_UINT },

            // BC1 maps to the RGBA variant, not BC1_RGB: both are 8 bytes per block and the
            // RGB one differs only in ignoring the 1-bit alpha, so the RGBA form is the
            // strictly more capable of the two at identical cost.
            { Format::BC1Unorm, VK_FORMAT_BC1_RGBA_UNORM_BLOCK },
            { Format::BC1Srgb, VK_FORMAT_BC1_RGBA_SRGB_BLOCK },
            { Format::BC3Unorm, VK_FORMAT_BC3_UNORM_BLOCK },
            { Format::BC3Srgb, VK_FORMAT_BC3_SRGB_BLOCK },
            { Format::BC4Unorm, VK_FORMAT_BC4_UNORM_BLOCK },
            { Format::BC5Unorm, VK_FORMAT_BC5_UNORM_BLOCK },
            { Format::BC6HFloat, VK_FORMAT_BC6H_UFLOAT_BLOCK },
            { Format::BC7Unorm, VK_FORMAT_BC7_UNORM_BLOCK },
            { Format::BC7Srgb, VK_FORMAT_BC7_SRGB_BLOCK },
        };

        static_assert(std::size(kFormats) == kFormatCount,
                      "The Format table has drifted from the enum. Add the row rather than letting "
                      "the new format fall through to VK_FORMAT_UNDEFINED.");

        consteval bool RowsMatchEnum() {
            for (usize i = 0; i < kFormatCount; ++i) {
                if (static_cast<usize>(kFormats[i].rhi) != i) return false;
            }

            return true;
        }

        static_assert(RowsMatchEnum(), "kFormats is not in Format declaration order; row i must carry enum value i");

        consteval usize ReverseSize() {
            usize hi = 0;

            for (const FormatPair &e : kFormats) {
                const usize v = static_cast<usize>(e.vk);
                if (v > hi) hi = v;
            }

            return hi + 1;
        }

        consteval auto MakeReverse() {
            // Value-initialised to 0 == Format::Undefined, which is the correct
            // answer for every VkFormat this backend does not name.
            std::array<Format, ReverseSize()> t{};

            for (const FormatPair &e : kFormats) t[static_cast<usize>(e.vk)] = e.rhi;

            return t;
        }

        // Sized by the largest core VkFormat named above. Extension formats are numbered
        // from 1000000000 and fall out through the bounds check in the lookup below.
        constexpr auto kReverseFormats = MakeReverse();

    } // namespace detail

    /**
     * @brief Maps an RHI format to the Vulkan format the backend creates resources with.
     * @param format Format to map.
     * @returns The matching VkFormat; VK_FORMAT_UNDEFINED for Format::Undefined.
     */
    [[nodiscard]] VE_INLINE constexpr VkFormat FromVulkyrieToVulkanFormat(Format format) noexcept {
        VASSERT(static_cast<usize>(format) < detail::kFormatCount, "Format index {} is out of range", static_cast<usize>(format));
        return detail::kFormats[static_cast<usize>(format)].vk;
    }

    /**
     * @brief Maps a Vulkan format back to the RHI format that names it.
     * @param format Format to map, from a surface or capability query.
     * @returns The matching Format, or Format::Undefined for anything the RHI does not name.
     */
    [[nodiscard]] VE_INLINE constexpr Format FromVulkanToVulkyrieFormat(VkFormat format) noexcept {
        const auto v = static_cast<usize>(format);
        if (v >= detail::kReverseFormats.size()) return Format::Undefined;
        return detail::kReverseFormats[v];
    }

} // namespace Vulkyrie
