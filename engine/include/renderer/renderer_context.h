#pragma once

#include "vlkypch.h"
#include "core/status_codes.h"
#include "core/graphics_api.h"

namespace Vulkyrie::Renderer {

    template <typename T> struct Handle {
        public:
            size_t Index;
            u32 Generation;
    };

    struct BufferHandle : public Handle<BufferHandle> {};

    class RendererContext {
        public:
            RendererContext(Vulkyrie::Core::GraphicsAPI api);

            RendererContext(const RendererContext &) = delete;
            RendererContext &operator=(const RendererContext &) = delete;

            RendererContext(RendererContext &&) = delete;
            RendererContext &operator=(RendererContext &&) = delete;

            virtual ~RendererContext() = default;

            /** @brief Initializes the graphics context.
             * @returns StatusCode indicating success or failure. */
            virtual Vulkyrie::Core::StatusCode Initialize() = 0;

            /** @brief Swaps the front and back buffers, presenting the rendered image to the screen. */
            inline virtual void SwapBuffers() {
            }

            /** @brief Creates a graphics context for the currently active graphics API.
             * @returns A smart pointer to the created GraphicsContext.
             */
            static Scope<RendererContext> Create();

            virtual BufferHandle CreateBuffer(std::span<f32> data) = 0;
            virtual BufferHandle CreateBuffer(size_t size, std::span<f32> data) = 0;
            virtual void SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) = 0;
            virtual void DestroyBuffer(const BufferHandle &handle) = 0;

        protected:
            RendererContext() = default;
    };
} // namespace Vulkyrie::Renderer
