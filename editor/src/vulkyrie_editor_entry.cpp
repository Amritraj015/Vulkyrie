#include <vulkyrie.h>
#include "vulkyrie_editor.h"

std::unique_ptr<Vulkyrie::Core::Application> CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps = {
        .Height = 800,
        .Width = 1500,
        .Title = "Vulkyrie Editor",
        .GraphicsAPI = Vulkyrie::Core::GraphicsAPI::OpenGL,
    };

    return std::make_unique<Vulkyrie::Editor::VulkyrieEditor>(windowProps);
}
