#include <vulkyrie.h>
#include "vulkyrie_editor.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    WindowProps windowProps = {
        .Height = 800,
        .Width = 1500,
        .Title = "Vulkyrie Editor",
        .GraphicsAPI = GraphicsAPI::OpenGL,
    };

    return new Vulkyrie::Editor::VulkyrieEditor(windowProps);
}
