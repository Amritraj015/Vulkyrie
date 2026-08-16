#pragma once

#include "core/graphics_api.h"
#include "core/types/static_string.h"
#include "renderer/open_gl/open_gl_command_list.h"
#include "renderer/open_gl/open_gl_context.h"
#include "renderer/open_gl/open_gl_pool.h"
#include "renderer/open_gl/open_gl_queue.h"
#include "renderer/open_gl/open_gl_swapchain.h"
#include "renderer/open_gl/open_gl_types.h"

namespace Vulkyrie {

    struct OpenGLBackend {
        // Types.
        using Context = OpenGLContext;
        using Queue = OpenGLQueue;
        using CommandList = OpenGLCommandList;
        using CommandPool = OpenGLPool;
        using Swapchain = OpenGLSwapchain;

        // Handles.
        using Image = OpenGLImage;
        using Buffer = OpenGLBuffer;
        using Sampler = OpenGLSampler;
        using Pipeline = OpenGLPipeline;
        using ShaderModule = OpenGLShaderModule;

        static constexpr StaticString kName = "OpenGL";
        static constexpr GraphicsAPI kType = GraphicsAPI::OpenGL;
        static constexpr u32 kFramesInFlight = 2;
        static constexpr bool kUsesBindlessHeap = false;
        static constexpr bool kHasTimelineSync = false;
        static constexpr bool kHasExplicitBarriers = false;
        static constexpr bool kHasMemoryAliasing = false;
        static constexpr bool kRecordsInParallel = false;
    };

} // namespace Vulkyrie
