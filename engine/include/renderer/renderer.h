#pragma once

#include "vlkypch.h"
#include "core/graphics_api.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    class Renderer {
    public:
        VE_DELETE_MOVE_AND_COPY(Renderer);

        virtual ~Renderer() = default;

        [[nodiscard]] static Scope<Renderer> Create(const GraphicsAPI api, const DeviceCreationInfo &info);

        [[nodiscard]] virtual GraphicsAPI BackendType() const noexcept = 0;
        [[nodiscard]] virtual const DeviceCapabilities &QueryCapabilities() const = 0;

        virtual void OnWindowResize(u32 width, u32 height) = 0;
        virtual void Render() = 0;
        virtual void WaitIdle() = 0;

        [[nodiscard]] virtual bool DeviceLost() const noexcept = 0;
        [[nodiscard]] virtual const RendererStatistics &GetStatistics() const noexcept = 0;

    protected:
        Renderer() = default;
    };

} // namespace Vulkyrie
