#pragma once

#include "vlkypch.h"
#include "core/time_step.h"
#include "renderer/rhi/constants.h"

namespace Vulkyrie {

    struct ViewInfo final {
        glm::mat4 ViewMatrix{ 0 };
        glm::mat4 ProjectionMatrix{ 0 };
        glm::vec3 CameraPosition{ 0 };
        f32 NearPlane = 0.1f;
        f32 FarPlane = 1000.0f;
        u32 ViewportWidth = 0;
        u32 ViewportHeight = 0;
    };

    struct DrawItem final {
        u32 MeshletRangeOffset = 0;
        u32 MeshletRangeCount = 0;
        u32 MaterialIndex = kInvalidRendererIndex;  // bindless, pre-resolved
        u32 TransformIndex = kInvalidRendererIndex; // index into transform SSBO
        u32 PipelineIndex = kInvalidRendererIndex;  // index into packet.pipelines
    };

    struct FramePacket final {
        ViewInfo MainView{};
        usize FrameIndex;
        Timestep deltaTime;
        std::span<const ViewInfo> ShadowViews{};
        std::span<const DrawItem> Opaque{};
        std::span<const DrawItem> Transparent{};
        u64 TransformBufferOffset = 0; // Base offset for transforms in the "mega" buffer.
        u64 MaterialBufferOffset = 0;  // Base offset for Materials in the "mega" buffer.
    };

} // namespace Vulkyrie
