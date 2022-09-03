#pragma once

#include "Defines.h"
#include "Renderer.h"
#include "Platform/Platform.h"
#include "Renderers/RendererType.h"

namespace Vkr
{
    struct RendererPacket
    {
        f32 deltaTime;
    };

    class RendererClient
    {

    public:
        CONSTRUCTOR_LOG(RendererClient)
        DESTRUCTOR_LOG(RendererClient)

        StatusCode Initialize(std::shared_ptr<Platform> platform, RendererType rendererType, const char *appName = "Vulkyrie Engine");

        StatusCode Terminate();

        void OnResize();

        StatusCode DrawFrame(RendererPacket *packet);

    private:
        std::unique_ptr<Renderer> renderer;

        StatusCode BeginFrame(f32 deltaTime);

        StatusCode EndFrame(f32 deltaTime);
    };
}