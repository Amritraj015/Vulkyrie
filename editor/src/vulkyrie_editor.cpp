#include "vulkyrie_editor.h"
#include "vulkyrie_layer_ui.h"

namespace Vulkyrie::Editor {
    VulkyrieEditor::VulkyrieEditor(const WindowProps &windowProps)
        : Application(windowProps) {
        PushOverlay<VulkyrieLayerUI>();
    }

} // namespace Vulkyrie::Editor
