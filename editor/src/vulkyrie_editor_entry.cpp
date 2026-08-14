#include <vulkyrie.h>
#include "vulkyrie_editor.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    ApplicationSettings settings = {
        .Name = "Vulkyrie Editor",
        .GraphicsSettings = {
            .API = GraphicsAPI::OpenGL,
            .WindowHeight = 800,
            .WindowWidth = 1500,
            .EnableVSync = false,
        },
    };

    return new Vulkyrie::Editor::VulkyrieEditor(settings);
}
