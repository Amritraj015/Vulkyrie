#pragma once

#include <vulkyrie.h>

namespace Vulkyrie::Editor {
    class VulkyrieEditor : public Application {
    public:
        VulkyrieEditor(const WindowProps &windowProps);
        ~VulkyrieEditor() override = default;
    };
} // namespace Vulkyrie::Editor
