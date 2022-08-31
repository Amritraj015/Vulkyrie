#include "DirectXRenderer.h"

namespace Vkr
{
    StatusCode DirectXRenderer::Initialize(const char *appName)
    {
        return StatusCode::Successful;
    }

    StatusCode DirectXRenderer::Shutdown()
    {
        return StatusCode::Successful;
    }

    void DirectXRenderer::OnResize(unsigned short width, unsigned short height)
    {
    }

    StatusCode DirectXRenderer::BeginFrame(float deltaTime)
    {
        return StatusCode::Successful;
    }

    StatusCode DirectXRenderer::EndFrame(float deltaTime)
    {
        return StatusCode::Successful;
    }
}