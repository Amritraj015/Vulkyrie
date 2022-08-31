#pragma once
#include "Core/Renderers/Renderer.h"

namespace Vkr
{
    class OpenGLRenderer : public Renderer
    {
    public:
        CONSTRUCTOR_LOG(OpenGLRenderer)
        DESTRUCTOR_LOG(OpenGLRenderer)

        StatusCode Initialize(const char *appName) override;
        StatusCode Shutdown() override;
        void OnResize(unsigned short width, unsigned short height) override;
        StatusCode BeginFrame(float deltaTime) override;
        StatusCode EndFrame(float deltaTime) override;
    };
}