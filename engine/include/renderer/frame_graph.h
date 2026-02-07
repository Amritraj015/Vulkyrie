#pragma once

namespace Vulkyrie::Renderer {

    class FrameGraph {
        public:
            FrameGraph() = default;
            ~FrameGraph() = default;

            FrameGraph(const FrameGraph &) = delete;
            FrameGraph &operator=(const FrameGraph &) = delete;

            FrameGraph(FrameGraph &&) = delete;
            FrameGraph &operator=(FrameGraph &&) = delete;
    };

} // namespace Vulkyrie::Renderer
