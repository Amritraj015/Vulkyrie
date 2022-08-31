#pragma once
#include "Defines.h"

namespace Vkr
{
    class Renderer
    {
    public:
        virtual ~Renderer() = default;

        virtual StatusCode Initialize(const char *appName) = 0;
        virtual StatusCode Shutdown() = 0;
        virtual void OnResize(unsigned short width, unsigned short height) = 0;
        virtual StatusCode BeginFrame(float deltaTime) = 0;
        virtual StatusCode EndFrame(float deltaTime) = 0;

    protected:
        unsigned long mFrameNumber;
    };
}