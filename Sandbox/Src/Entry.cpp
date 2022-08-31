#include "SandboxApp.h"

Vkr::Application *GetApplication()
{
    static SandboxApp sConfig;

    sConfig.startX = 100;
    sConfig.startY = 100;
    sConfig.width = 400;
    sConfig.height = 400;
    sConfig.rendererType = Vkr::RendererType::Vulkan;
    sConfig.name = "Vulkyrie Engine";

    return &sConfig;
}