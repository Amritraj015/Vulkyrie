#pragma once

#include "core/status_codes.h"
#include "core/platform.h"
#include "renderer/scene.h"
#include "renderer/polygon_fill_mode.h"

namespace Vulkyrie::Renderer {
    class Renderer {
        public:
            Renderer(const Vulkyrie::Core::Platform &platform);

            Vulkyrie::Core::StatusCode Initialize();
            Vulkyrie::Core::StatusCode Terminate();

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
