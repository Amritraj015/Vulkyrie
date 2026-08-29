#pragma once

#include "renderer/rhi/formats.h"
#include "renderer/rhi/rhi_types.h"
#include "renderer/rhi/shader_types.h"

namespace Vulkyrie {

    enum class PrimitiveTopology : u8 { PointList, LineList, LineStrip, TriangleList, TriangleStrip };
    enum class CullMode : u8 { None, Front, Back };
    enum class FrontFace : u8 { CounterClockwise, Clockwise };
    enum class PolygonFillMode : u8 { Fill, Line, Point };
    enum class BlendOp : u8 { Add, Subtract, ReverseSubtract, Min, Max };
    enum class BlendFactor : u8 {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate,
        Src1Color,
        OneMinusSrc1Color,
        Src1Alpha,
        OneMinusSrc1Alpha,
    };

    struct RasterState final {
        f32 DepthBiasConstant = 0.0f;
        f32 DepthBiasSlope = 0.0f;
        CullMode Cull = CullMode::Back;
        FrontFace FrontFace = FrontFace::CounterClockwise;
        PolygonFillMode FillMode = PolygonFillMode::Fill;
        bool DepthClamp = false;
        bool DepthBiasEnabled = false;
    };

    struct DepthStencilState final {
        bool DepthTest = true;
        bool DepthWrite = true;
        CompareOp DepthCompare = CompareOp::GreaterEqual;
        bool StencilTest = false;
    };

    struct BlendState final {
        bool Enable = false;
        BlendFactor SrcColor = BlendFactor::One;
        BlendFactor DstColor = BlendFactor::Zero;
        BlendOp ColorOp = BlendOp::Add;
        BlendFactor SrcAlpha = BlendFactor::One;
        BlendFactor DstAlpha = BlendFactor::Zero;
        BlendOp AlphaOp = BlendOp::Add;
        u8 WriteMask = 0xF;
    };

    inline constexpr u32 MAX_COLOR_ATTACHMENTS = 8;

    struct RenderTargetLayout final {
        Format ColorFormats[MAX_COLOR_ATTACHMENTS]{};
        u32 ColorCount = 0;
        Format DepthFormat = Format::Undefined;
        SampleCount Samples = SampleCount::None;
    };

    struct GraphicsPipelineDescriptor final {
        ShaderKey VertexShader{};
        ShaderKey FragmentShader{};
        ShaderKey TaskShader{}; // optional, mesh pipeline
        ShaderKey MeshShader{}; // optional, mesh pipeline

        PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
        RasterState Raster{};
        DepthStencilState DepthStencil{};
        BlendState Blends[MAX_COLOR_ATTACHMENTS]{};
        RenderTargetLayout RenderTargetLayout{};
        u32 PushConstantBytes = 0;

        // StaticString DebugName; // not hashed
    };

    struct ComputePipelineDescriptor final {
        ShaderKey ComputeShader{};
        u32 PushConstantBytes = 0;

        // StaticString DebugName; // not hashed
    };

    [[nodiscard]] VE_INLINE constexpr u64 HashDescriptor(const GraphicsPipelineDescriptor &d) noexcept {
        HashBuilder hb;

        hb.Value(d.VertexShader.SourceHash)
            .Value(d.VertexShader.DefineHash)
            .Value(d.VertexShader.ShaderStage)
            .Value(d.VertexShader.ShaderTarget)
            .Value(d.FragmentShader.SourceHash)
            .Value(d.FragmentShader.DefineHash)
            .Value(d.FragmentShader.ShaderStage)
            .Value(d.FragmentShader.ShaderTarget)
            .Value(d.TaskShader.SourceHash)
            .Value(d.TaskShader.DefineHash)
            .Value(d.TaskShader.ShaderStage)
            .Value(d.TaskShader.ShaderTarget)
            .Value(d.MeshShader.SourceHash)
            .Value(d.MeshShader.DefineHash)
            .Value(d.MeshShader.ShaderStage)
            .Value(d.MeshShader.ShaderTarget)
            .Value(d.Topology)
            .Value(d.Raster.DepthBiasConstant)
            .Value(d.Raster.DepthBiasSlope)
            .Value(d.Raster.Cull)
            .Value(d.Raster.FrontFace)
            .Value(d.Raster.FillMode)
            .Value(d.Raster.DepthClamp)
            .Value(d.Raster.DepthBiasEnabled)
            .Value(d.DepthStencil.DepthTest)
            .Value(d.DepthStencil.DepthWrite)
            .Value(d.DepthStencil.DepthCompare)
            .Value(d.DepthStencil.StencilTest);

        for (usize i = 0; i < MAX_COLOR_ATTACHMENTS; ++i) {
            hb.Value(d.Blends[i].Enable)
                .Value(d.Blends[i].SrcColor)
                .Value(d.Blends[i].DstColor)
                .Value(d.Blends[i].ColorOp)
                .Value(d.Blends[i].SrcAlpha)
                .Value(d.Blends[i].DstAlpha)
                .Value(d.Blends[i].AlphaOp)
                .Value(d.Blends[i].WriteMask);
        }

        for (usize i = 0; i < MAX_COLOR_ATTACHMENTS; ++i) {
            hb.Value(d.RenderTargetLayout.ColorFormats[i]);
        }

        return hb.Value(d.RenderTargetLayout.ColorCount)
            .Value(d.RenderTargetLayout.DepthFormat)
            .Value(d.RenderTargetLayout.Samples)
            .Value(d.PushConstantBytes)
            .Finish();
    }

    [[nodiscard]] VE_INLINE constexpr u64 HashDescriptor(const ComputePipelineDescriptor &d) noexcept {
        HashBuilder hb;

        return hb.Value(d.ComputeShader.SourceHash)
            .Value(d.ComputeShader.DefineHash)
            .Value(d.ComputeShader.ShaderStage)
            .Value(d.ComputeShader.ShaderTarget)
            .Value(d.PushConstantBytes)
            .Finish();
    }

} // namespace Vulkyrie
