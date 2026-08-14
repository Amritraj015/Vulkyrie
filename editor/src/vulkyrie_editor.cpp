#include "vulkyrie_editor.h"
#include "vulkyrie_layer_ui.h"

namespace Vulkyrie::Editor {

    VulkyrieEditor::VulkyrieEditor(const ApplicationSettings &settings)
        : Application(settings) {
        PushOverlay<VulkyrieLayerUI>();
    }

} // namespace Vulkyrie::Editor
