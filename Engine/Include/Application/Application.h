#pragma once

#include "Renderers/RendererType.h"

namespace Vkr
{
    class Application
    {
    public:
        virtual ~Application() = default;

        // Starting position of the window on x-axis, if applicable.
        unsigned short startX{};

        // Starting position of the window on y-axis, if applicable.
        unsigned short startY{};

        // Starting width of the window, if applicable.
        unsigned short width{};

        // Starting height of the window, if applicable.
        unsigned short height{};

        // The application name on the window, if applicable.
        const char *name{};

        // Type of renderer to use for this application.
        RendererType rendererType = RendererType::Vulkan;

        // Function pointer to the application's initialize function.
        virtual bool Initialize() = 0;

        // Function pointer to the application's update function.
        virtual bool Update(float delta_time) = 0;

        // Function pointer to the application's render function.
        virtual bool Render(float delta_time) = 0;

        // Function pointer that handles resizes, if applicable.
        virtual void OnResize(unsigned short width, unsigned short height) = 0;
    };
}
