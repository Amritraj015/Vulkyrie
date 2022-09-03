#pragma once
#include "Defines.h"

namespace Vkr
{
    // Base renderer class
    class Renderer
    {
    public:
        virtual ~Renderer() = default;

        virtual StatusCode Initialize(const char *appName) = 0; // Initializes the renderer.
        virtual StatusCode Shutdown() = 0;                      // Shuts the renderer down.
        virtual void OnResize(u16 width, u16 height) = 0;       // Callback function to execute on window resize.
        virtual StatusCode BeginFrame(f32 deltaTime) = 0;       // Callback function to execute when a frame begins.
        virtual StatusCode EndFrame(f32 deltaTime) = 0;         // Callback function to execute when a frame ends.

    protected:
        unsigned long mFrameNumber;
    };
}