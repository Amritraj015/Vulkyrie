#pragma once
#include "Renderers/Renderer.h"

namespace Vkr
{
    class DirectXRenderer : public Renderer
    {
    public:
        CONSTRUCTOR_LOG(DirectXRenderer)
        DESTRUCTOR_LOG(DirectXRenderer)

        StatusCode Initialize(const char *appName) override;
        StatusCode Shutdown() override;
        void OnResize(u16 width, u16 height) override;
        StatusCode BeginFrame(f32 deltaTime) override;
        StatusCode EndFrame(f32 deltaTime) override;
    };
}