#include <vulkyrie.h>
#include "vulkyrie_editor.h"

using namespace Vulkyrie;

std::unique_ptr<Application> CreateApplication() {
    WindowProps windowProps = {
        .Height = 800,
        .Width = 1500,
        .Title = "Vulkyrie Editor",
        .GraphicsAPI = GraphicsAPI::OpenGL,
    };

    return std::make_unique<Vulkyrie::Editor::VulkyrieEditor>(windowProps);
}
