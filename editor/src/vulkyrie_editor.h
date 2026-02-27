#pragma once

#include <vulkyrie.h>

namespace Vulkyrie::Editor {
    class VulkyrieEditor : public Vulkyrie::Core::Application {
        public:
            VulkyrieEditor(const Vulkyrie::Core::WindowProps &windowProps);
            ~VulkyrieEditor() override = default;
    };
} // namespace Vulkyrie::Editor
