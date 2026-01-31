#pragma once

#include "core/status_codes.h"
#include "core/platform.h"
#include "renderer/scene.h"
#include "renderer/polygon_fill_mode.h"

namespace Vulkyrie::Renderer {
    struct RendererStatistics {
        public:
            u32 FramesRendered = 0;
            u32 DrawCalls = 0;
            u32 TrianglesRendered = 0;
    };

    /** @brief Initializes the renderer with the specified graphics API.
     * @param api The graphics API to initialize the renderer with.
     * @returns StatusCode indicating success or failure.
     */
    Vulkyrie::Core::StatusCode Initialize(Vulkyrie::Core::GraphicsAPI api);

    /** @brief Gets the current graphics API being used by the renderer.
     * @returns The current graphics API.
     */
    Vulkyrie::Core::GraphicsAPI GetCurrentGraphicsAPI();

    /** @brief Gets the name of the current graphics API as a string view.
     * @returns A string view representing the name of the current graphics API.
     */
    std::string_view GetCurrentGraphicsAPIName();

    // void SetViewport(u32 x, u32 y, u32 width, u32 height);

    class Renderer {
        public:
            Renderer(const Vulkyrie::Core::Platform &platform);

            void BeginScene(const Scene &scene);
            void EndScene();

            /** @brief Handles window resize events.
             * @param width The new width of the window.
             * @param height The new height of the window.
             */
            void OnWindowResize(u32 width, u32 height);

            /** @brief Sets the polygon fill mode for rendering.
             * @param mode The polygon fill mode to set.
             */
            void SetPolygonFillMode(PolygonFillMode mode);

            void Submit();

        private:
            const Vulkyrie::Core::Platform &_platform;
    };
} // namespace Vulkyrie::Renderer
