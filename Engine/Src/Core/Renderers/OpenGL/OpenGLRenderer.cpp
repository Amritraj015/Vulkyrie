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

    void OpenGLRenderer::OnResize(unsigned short width, unsigned short height)
    {
    }

    StatusCode OpenGLRenderer::BeginFrame(float deltaTime)
    {
        return StatusCode::Successful;
    }

    StatusCode OpenGLRenderer::EndFrame(float deltaTime)
    {
        return StatusCode::Successful;
    }
}