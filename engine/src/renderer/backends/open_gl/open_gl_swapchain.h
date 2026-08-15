#pragma once

#include "core/types/application_types.h"
#include "renderer/backends/open_gl/open_gl_context.h"

namespace Vulkyrie {

    class OpenGLSwapchain {
    public:
        struct AcquiredImage final {
            OpenGLImage Image{};
            u32 Index = 0;
            bool Suboptimal = false;
        };

        OpenGLSwapchain() = default;
        OpenGLSwapchain(OpenGLContext *context, const WindowHandle &windowHandle, u32 width, u32 height, bool vSync);
        ~OpenGLSwapchain() = default;

        [[nodiscard]] AcquiredImage Acquire(u64 timeoutNs = std::numeric_limits<u64>::max());
        [[nodiscard]] bool Present(u32 imageIndex, u32 renderFinished);
        void Recreate(u32 width, u32 height, bool vSync);

        [[nodiscard]] VE_INLINE Format Format() const noexcept {
            return mFormat;
        }

        [[nodiscard]] VE_INLINE u32 Height() const noexcept {
            return mHeight;
        }

        [[nodiscard]] VE_INLINE u32 Width() const noexcept {
            return mWidth;
        }

        [[nodiscard]] VE_INLINE bool VSyncEnabled() const noexcept {
            return mVSync;
        }

        [[nodiscard]] VE_INLINE constexpr u32 ImageCount() const noexcept {
            return 1;
        }

    private:
        [[maybe_unused]] OpenGLContext *pContext;
        enum Format mFormat;
        u32 mWidth;
        u32 mHeight;
        bool mVSync;
    };

} // namespace Vulkyrie
