#include "OpenGLRenderer.h"

namespace Vkr
{
    StatusCode OpenGLRenderer::Initialize(const char *appName)
    {
        return StatusCode::Successful;
    }

    StatusCode OpenGLRenderer::Shutdown()
    {
        return StatusCode::Successful;
    }

    void OpenGLRenderer::OnResize(u16 width, u16 height)
    {
    }

    StatusCode OpenGLRenderer::BeginFrame(f32 deltaTime)
    {
        return StatusCode::Successful;
    }

    StatusCode OpenGLRenderer::EndFrame(f32 deltaTime)
    {
        return StatusCode::Successful;
    }
}