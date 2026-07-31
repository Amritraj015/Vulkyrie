#pragma once

#include "vlkypch.h"
#include "core/status_codes.h"
#include "core/graphics_api.h"
#include "renderer/backends/renderer_backend.h"

namespace Vulkyrie {

    class RendererContext final {
    public:
        RendererContext() = delete;

        VE_DELETE_MOVE_AND_COPY(RendererContext);

        [[nodiscard]] static StatusCode Create(GraphicsAPI graphicsApi);
        [[nodiscard]] static StatusCode SwitchTo(GraphicsAPI otherGraphicsApi);
        static void Dispose();

        [[nodiscard]] static constexpr VE_INLINE GraphicsAPI GetCurrentGraphicsAPI() {
            return _currentGraphicsApi;
        }

        [[nodiscard]] static constexpr std::string_view GetCurrentGraphicsAPIName() {
            switch (_currentGraphicsApi) {
                case GraphicsAPI::OpenGL:
                    return "OpenGL";
                case GraphicsAPI::Vulkan:
                    return "Vulkan";
                default:
                    return "Unknown";
            }
        }

    private:
        static GraphicsAPI _currentGraphicsApi;
        static bool _initialized;
        static RendererBackend *_rendererBackend;
    };

} // namespace Vulkyrie
