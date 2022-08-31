#pragma once

#include "Defines.h"
#include <Renderers/RendererType.h>
#include "Renderer.h"

namespace Vkr
{
    struct RendererPacket
    {
        float deltaTime;
    };

    class RendererClient
    {

    public:
        CONSTRUCTOR_LOG(RendererClient)
        DESTRUCTOR_LOG(RendererClient)

        StatusCode Initialize(RendererType rendererType, const char *appName = "Vulkyrie Engine");

        StatusCode Terminate();

        void OnResize();

        StatusCode DrawFrame(RendererPacket *packet);

    private:
        std::unique_ptr<Renderer> renderer;

        StatusCode BeginFrame(float deltaTime);

        StatusCode EndFrame(float deltaTime);
    };
}