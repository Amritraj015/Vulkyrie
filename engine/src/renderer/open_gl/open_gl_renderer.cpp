#include "renderer/renderer.h"
#include "core/logger.h"

namespace Vulkyrie::Renderer {
    Renderer::Renderer(const Vulkyrie::Core::Platform &platform) : _platform(platform) {
    }

    Vulkyrie::Core::StatusCode Renderer::Initialize() {

        return Vulkyrie::Core::StatusCode::Successful;
    }

    Vulkyrie::Core::StatusCode Renderer::Terminate() {
        return Vulkyrie::Core::StatusCode::Successful;
    }

    void Renderer::BeginScene() {
    }

    void Renderer::EndScene() {
    }
} // namespace Vulkyrie::Renderer
