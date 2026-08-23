#include <vulkyrie.h>
#include "vulkyrie_editor.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    const Vulkyrie::ApplicationInfo appInfo{
        .Name = "Vulkyrie Editor",
        .MajorVersion = 1,
        .MinorVersion = 0,
        .PatchVersion = 0,
    };

    const ApplicationSettings settings = {
        .Application = appInfo,
        .GraphicsSettings = {
            .API = GraphicsAPI::OpenGL,
            .WindowHeight = 800,
            .WindowWidth = 1500,
            .EnableVSync = false,
        },
    };

    return new Vulkyrie::Editor::VulkyrieEditor(settings);
}
