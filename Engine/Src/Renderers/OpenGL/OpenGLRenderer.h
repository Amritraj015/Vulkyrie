#pragma once
#include "Renderers/Renderer.h"

namespace Vkr
{
    class OpenGLRenderer : public Renderer
    {
    public:
        CONSTRUCTOR_LOG(OpenGLRenderer)
        DESTRUCTOR_LOG(OpenGLRenderer)

        StatusCode Initialize(const char *appName) override;
        StatusCode Shutdown() override;
        void OnResize(u16 width, u16 height) override;
        StatusCode BeginFrame(f32 deltaTime) override;
        StatusCode EndFrame(f32 deltaTime) override;
    };
}