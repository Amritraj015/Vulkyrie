#pragma once

#include "core/status_codes.h"
#include "core/graphics_api.h"
#include "renderer/camera.h"
#include "renderer/polygon_fill_mode.h"

namespace Vulkyrie::Renderer {
    /** @brief Renderer statistics structure to hold performance metrics. */
    struct RendererStatistics {
        public:
            /** @brief Number of frames rendered during rendering. */
            u32 FramesRendered = 0;

            /** @brief Number of draw calls made during rendering. */
            u32 DrawCalls = 0;

            /** @brief Number of triangles rendered during rendering. */
            u32 TrianglesRendered = 0;
    };

    /** @brief Initializes the renderer with the specified graphics API.
     * @param api The graphics API to initialize the renderer with.
     * @returns StatusCode indicating success or failure.
     */
    [[nodiscard]] Vulkyrie::Core::StatusCode Initialize(Vulkyrie::Core::GraphicsAPI api);

    /** @brief Gets the current graphics API being used by the renderer.
     * @returns The current graphics API.
     */
    [[nodiscard]] Vulkyrie::Core::GraphicsAPI GetCurrentGraphicsAPI();

    /** @brief Gets the name of the current graphics API as a string view.
     * @returns A string view representing the name of the current graphics API.
     */
    [[nodiscard]] std::string_view GetCurrentGraphicsAPIName();

    // void SetViewport(u32 x, u32 y, u32 width, u32 height);

    class CommandBuffer {
        public:
            virtual ~CommandBuffer() = default;

            virtual void Begin() = 0;
            virtual void End() = 0;
    };

    class Renderer {
        public:
            virtual ~Renderer() = default;

            virtual void BeginScene(const Camera &camera) = 0;
            virtual void EndScene() = 0;
            virtual Scope<CommandBuffer> CreateCommandBuffer() = 0;

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
    };
} // namespace Vulkyrie::Renderer
