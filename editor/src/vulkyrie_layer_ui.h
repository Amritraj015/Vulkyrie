#pragma once

#include <vulkyrie.h>

namespace Vulkyrie::Editor {
    using namespace Vulkyrie::Core;
    using namespace Vulkyrie::Events;

    class VulkyrieLayerUI final : public Layer {
        public:
            ~VulkyrieLayerUI() override = default;

            void OnAttached() override;
            void OnDetached() override;
            void OnUpdate(Timestep deltaTime) override;
            void OnEvent(Event &event) override;
    };
} // namespace Vulkyrie::Editor
