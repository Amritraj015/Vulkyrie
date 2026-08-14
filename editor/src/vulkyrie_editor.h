#pragma once

#include <vulkyrie.h>

namespace Vulkyrie::Editor {

    class VulkyrieEditor : public Application {
    public:
        VulkyrieEditor(const ApplicationSettings &settings);
        ~VulkyrieEditor() override = default;
    };

} // namespace Vulkyrie::Editor
