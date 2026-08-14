#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    template <typename TResource> struct RendererResourceHandle {
    public:
        static constexpr u32 INVALID_RENDERER_RESOURCE_INDEX = std::numeric_limits<u32>::max();

        constexpr RendererResourceHandle() = default;

        constexpr RendererResourceHandle(u32 index, u32 generation) noexcept
            : _index(index)
            , _generation(generation) {
        }

        [[nodiscard]] constexpr VE_INLINE u32 Index() const {
            return _index;
        }

        [[nodiscard]] constexpr VE_INLINE u32 Generation() const {
            return _generation;
        }

        /** @brief Checks whether the handle belongs to a valid resource rather than being the invalid sentinel. */
        [[nodiscard]] VE_INLINE constexpr bool IsValid() const noexcept {
            return _index != INVALID_RENDERER_RESOURCE_INDEX;
        }

        friend constexpr bool operator==(RendererResourceHandle, RendererResourceHandle) = default;

    private:
        u32 _index = INVALID_RENDERER_RESOURCE_INDEX;
        u32 _generation = 0;
    };

    using BufferHandle = RendererResourceHandle<struct BufferTag>;
    using IndexBufferHandle = RendererResourceHandle<struct IndexBufferTag>;
    using TextureHandle = RendererResourceHandle<struct TextureTag>;
    using SamplerHandle = RendererResourceHandle<struct SamplerTag>;
    using PipelineHandle = RendererResourceHandle<struct PipelineTag>;

} // namespace Vulkyrie
