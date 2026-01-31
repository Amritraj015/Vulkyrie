#include "core/status_codes.h"
#include "renderer/renderer.h"
#include <glad/glad.h>

namespace Vulkyrie::Renderer {
    Renderer::Renderer(const Vulkyrie::Core::Platform &platform)
        : _platform(platform) {}

    // Vulkyrie::Core::StatusCode Renderer::Terminate() {
    //     return Vulkyrie::Core::StatusCode::Successful;
    // }

    // void Renderer::BeginScene(const Scene &scene) {
    // }

    void Renderer::OnWindowResize(u32 width, u32 height) { glViewport(0, 0, width, height); }

    void Renderer::SetPolygonFillMode(PolygonFillMode mode) {
        switch (mode) {
            case PolygonFillMode::Line:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;
            case PolygonFillMode::Point:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                break;
            case PolygonFillMode::Fill:
            default:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
        }
    }

    void Renderer::EndScene() {}
} // namespace Vulkyrie::Renderer
