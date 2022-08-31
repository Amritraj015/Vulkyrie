#include "RendererClient.h"
#include "Vulkan/VulkanRenderer.h"
#include "OpenGL/OpenGLRenderer.h"
#include "DiretX/DirectXRenderer.h"

namespace Vkr
{
    StatusCode RendererClient::Initialize(RendererType rendererType, const char *appName)
    {
        switch (rendererType)
        {
        case RendererType::Vulkan:
            renderer = std::make_unique<VulkanRenderer>();
            break;
        case RendererType::OpenGL:
            renderer = std::make_unique<OpenGLRenderer>();
            break;
        case RendererType::DirectX:
            renderer = std::make_unique<DirectXRenderer>();
            break;
        default:
            renderer = std::make_unique<VulkanRenderer>();
            break;
        }

        StatusCode statusCode = renderer->Initialize(appName);
        ENSURE_SUCCESS(statusCode, "Renderer backend failed to initialize.")

        return statusCode;
    }

    StatusCode RendererClient::Terminate()
    {
        return renderer->Shutdown();
    }

    void RendererClient::OnResize()
    {
    }

    StatusCode RendererClient::DrawFrame(RendererPacket *packet)
    {
        return StatusCode::Successful;
    }

    StatusCode RendererClient::BeginFrame(float deltaTime)
    {
        return StatusCode::Successful;
    }

    StatusCode RendererClient::EndFrame(float deltaTime)
    {
        return StatusCode::Successful;
    }
}