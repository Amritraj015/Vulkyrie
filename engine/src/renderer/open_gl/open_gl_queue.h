#pragma once

#include "vlkypch.h"
#include "renderer/open_gl/open_gl_command_list.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    class OpenGLQueue {
    public:
        void Submit(std::span<const OpenGLCommandList *const> lists, std::span<u64> waits, std::span<u64> signals);
        QueueType Type() const noexcept;
    };

} // namespace Vulkyrie
