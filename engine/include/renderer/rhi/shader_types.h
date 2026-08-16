#pragma once

#include "core/utilities/hash_builder.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    enum class ShaderTarget : u8 { SpirV, GlslEs, Count };

    struct ShaderKey final {
        u64 SourceHash = 0;
        u64 DefineHash = 0;
        ShaderStage ShaderStage = ShaderStage::Count;
        ShaderTarget ShaderTarget = ShaderTarget::Count;

        [[nodiscard]] VE_INLINE constexpr bool Valid() const noexcept {
            return 0 != SourceHash;
        }

        friend bool operator==(const ShaderKey &, const ShaderKey &) = default;
    };

    [[nodiscard]] constexpr u64 HashShaderKey(const ShaderKey &k) noexcept {
        HashBuilder hb;

        return hb.Value(k.SourceHash).Value(k.DefineHash).Value(k.ShaderStage).Value(k.ShaderTarget).Finish();
    }

    struct ShaderKeyHasher {
        [[nodiscard]] constexpr usize operator()(const ShaderKey &k) const noexcept {
            return static_cast<usize>(HashShaderKey(k));
        }
    };

} // namespace Vulkyrie
