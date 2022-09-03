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

    void DirectXRenderer::OnResize(u16 width, u16 height)
    {
    }

    StatusCode DirectXRenderer::BeginFrame(f32 deltaTime)
    {
        return StatusCode::Successful;
    }

    StatusCode DirectXRenderer::EndFrame(f32 deltaTime)
    {
        return StatusCode::Successful;
    }
}