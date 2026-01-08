#pragma once

#include "core/status_codes.h"
#include "core/platform.h"

namespace Vulkyrie::Renderer {
    class Renderer {
        public:
            Renderer(const Vulkyrie::Core::Platform &platform);

            Vulkyrie::Core::StatusCode Initialize();
            Vulkyrie::Core::StatusCode Terminate();

            void BeginScene();
            void EndScene();

        private:
            const Vulkyrie::Core::Platform &_platform;
    };
} // namespace Vulkyrie::Renderer
