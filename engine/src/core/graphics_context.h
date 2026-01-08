#pragma once

#include "vlkypch.h"
#include "core/graphics_api.h"
#include "core/status_codes.h"

namespace Vulkyrie::Core {
    class GraphicsContext {
        public:
            virtual ~GraphicsContext() = default;

            /** @brief Initializes the graphics context.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure. */
            virtual StatusCode Initialize() = 0;

            /** @brief Swaps the front and back buffers, presenting the rendered image to the screen. */
            inline virtual void SwapBuffers() = 0;

            /** @brief Creates a graphics context for the given window handle.
             * @param api The graphics API to use for the context.
             * @param windowHandle The handle to the window.
             * @returns A smart pointer to the created GraphicsContext.
             */
            static Scope<GraphicsContext> Create(Vulkyrie::Core::GraphicsAPI api, void *windowHandle);
    };
} // namespace Vulkyrie::Core
