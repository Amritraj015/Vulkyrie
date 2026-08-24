#include <vulkyrie.h>
#include "vulkyrie_editor.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    const Vulkyrie::ApplicationInfo appInfo{
        .Name = "Vulkyrie Editor",
        .Version = { 1, 0, 0 },
    };

    const ApplicationSettings settings = {
        .GeneralSettings = appInfo,
        .GraphicsSettings = {
            .ValidationSettings = {},
            .WindowDimensions = { 1500, 800 },
            .API = GraphicsAPI::OpenGL,
            .EnableVSync = false,
        },
    };

    return new Vulkyrie::Editor::VulkyrieEditor(settings);
}
