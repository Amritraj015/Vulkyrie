#pragma once

#include "vlkypch.h"
#include "core/status_codes.h"

namespace Vulkyrie::Renderer {
    class GraphicsContext {
        public:
            GraphicsContext(const GraphicsContext &) = delete;
            GraphicsContext &operator=(const GraphicsContext &) = delete;

            GraphicsContext(GraphicsContext &&) = delete;
            GraphicsContext &operator=(GraphicsContext &&) = delete;

            virtual ~GraphicsContext() = default;

            /** @brief Initializes the graphics context.
             * @returns StatusCode indicating success or failure. */
            virtual Vulkyrie::Core::StatusCode Initialize() = 0;

            /** @brief Swaps the front and back buffers, presenting the rendered image to the screen. */
            inline virtual void SwapBuffers() {
            }

            /** @brief Creates a graphics context for the currently active graphics API.
             * @returns A smart pointer to the created GraphicsContext.
             */
            static Scope<GraphicsContext> Create();

        protected:
            GraphicsContext() = default;
    };
} // namespace Vulkyrie::Renderer
