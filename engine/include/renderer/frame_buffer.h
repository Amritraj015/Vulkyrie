#pragma once

#include "renderer/texture_sampler_wrap_mode.h"
#include "renderer/texture_filter_mode.h"

namespace Vulkyrie::Renderer {
    /** @brief Types of attachments for framebuffers. */
    enum class AttachmentType : u32 {
        /** @brief Attachment is a texture. */
        Texture,

        /** @brief Attachment is a renderbuffer. */
        RenderBuffer,
    };

    /** @brief Supported color formats for framebuffer attachments. */
    enum class ColorFormat : u32 {
        /** @brief 8-bit RGBA format. */
        RGBA8,

        /** @brief 16-bit floating-point RGBA format. */
        RGBA16F,

        /** @brief 32-bit floating-point RGBA format. */
        RGBA32F,

        /** @brief 32-bit integer Red channel format. */
        R32I,
    };

    /** @brief Supported depth and stencil formats for framebuffer attachments. */
    enum class DepthStencilFormat : u32 {
        /** @brief 24-bit depth and 8-bit stencil format. */
        Depth24Stencil8,

        /** @brief 32-bit floating-point depth format. */
        Depth32F,
    };

    /** @brief Load operations for framebuffer attachments. */
    enum class LoadOp : u32 {
        /** @brief Preserve contents from previous rendering. */
        Load,

        /** @brief Clear contents at the start of rendering. */
        Clear,

        /** @brief Discard contents at the start of rendering. */
        DontCare,
    };

    /** @brief Store operations for framebuffer attachments. */
    enum class StoreOp : u32 {
        /** @brief Preserve contents after rendering. */
        Store,

        /** @brief Discard contents after rendering. */
        DontCare,
    };

    /** @brief Specification structure for color attachment. */
    struct ColorAttachmentSpecification {
        public:
            /** @brief Format of the color attachment. */
            ColorFormat Format;

            /** @brief Type of the attachment (Texture or RenderBuffer). */
            AttachmentType Type = AttachmentType::Texture;

            /** @brief Load operation for the attachment. */
            LoadOp Load = LoadOp::Clear;

            /** @brief Store operation for the attachment. */
            StoreOp Store = StoreOp::Store;

            /** @brief Minification filter for the attachment (only applicable if attachment is a texture). */
            TextureFilterMode MinFilter = TextureFilterMode::Linear;

            /** @brief Magnification filter for the attachment (only applicable if attachment is a texture). */
            TextureFilterMode MagFilter = TextureFilterMode::Linear;

            /** @brief Wrapping mode for the S (horizontal) texture coordinate (only applicable if attachment is a texture). */
            TextureSamplerWrapMode WrapS = TextureSamplerWrapMode::ClampToEdge;

            /** @brief Wrapping mode for the T (vertical) texture coordinate (only applicable if attachment is a texture). */
            TextureSamplerWrapMode WrapT = TextureSamplerWrapMode::ClampToEdge;

            /** @brief Number of samples for multi-sampling (default is 1, meaning no multi-sampling). */
            u32 Samples = 1;

            /** @brief Clear color value for the attachment. */
            f32 ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    /** @brief Specification structure for depth attachment. */
    struct DepthStencilAttachmentSpecification {
        public:
            /** @brief Format of the depth/stencil attachment. */
            DepthStencilFormat Format;

            /** @brief Type of the attachment (Texture or RenderBuffer). */
            AttachmentType Type = AttachmentType::Texture;

            /** @brief Load operation for the attachment. */
            LoadOp Load = LoadOp::Clear;

            /** @brief Store operation for the attachment. */
            StoreOp Store = StoreOp::Store;

            /** @brief Minification filter for the attachment (only applicable if attachment is a texture). */
            TextureFilterMode MinFilter = TextureFilterMode::Linear;

            /** @brief Magnification filter for the attachment (only applicable if attachment is a texture). */
            TextureFilterMode MagFilter = TextureFilterMode::Linear;

            /** @brief Wrapping mode for the S (horizontal) texture coordinate (only applicable if attachment is a texture). */
            TextureSamplerWrapMode WrapS = TextureSamplerWrapMode::ClampToEdge;

            /** @brief Wrapping mode for the T (vertical) texture coordinate (only applicable if attachment is a texture). */
            TextureSamplerWrapMode WrapT = TextureSamplerWrapMode::ClampToEdge;

            /** @brief Number of samples for multi-sampling (default is 1, meaning no multi-sampling). */
            u32 Samples = 1;

            /** @brief Clear value for depth. */
            f32 ClearDepth = 1.0f;

            /** @brief Clear value for stencil. */
            u32 ClearStencil = 0;
    };

    /** @brief Specification structure for creating a Frame Buffer. */
    struct FrameBufferSpecification {
        public:
            /** @brief Width of the framebuffer in pixels. */
            u32 Width = 0;

            /** @brief Height of the framebuffer in pixels. */
            u32 Height = 0;

            /** @brief Specifications for color attachments. */
            std::optional<std::vector<ColorAttachmentSpecification>> ColorAttachments = std::nullopt;

            /** @brief Specification for the depth and stencil attachments. */
            std::optional<DepthStencilAttachmentSpecification> DepthStencilAttachment = std::nullopt;

            /** @brief Indicates if the framebuffer is a swapchain target. */
            bool SwapchainTarget = false;

            /** @brief Optional debug name for the framebuffer. */
            std::string_view DebugName = "Unnamed FrameBuffer";
    };

    /** @brief Abstract base class for a Frame Buffer. */
    class FrameBuffer {
        public:
            virtual ~FrameBuffer() = default;

            /** @brief Resizes the framebuffer to the specified width and height.
             * @param width The new width of the framebuffer.
             * @param height The new height of the framebuffer.
             */
            virtual void Resize(u32 width, u32 height) = 0;

            /** @brief Binds the framebuffer for rendering. */
            virtual void Bind() const = 0;

            /** @brief Unbinds the framebuffer, reverting to the default framebuffer. */
            virtual void Unbind() const = 0;

            /** @brief Retrieves the resource ID of the specified color attachment.
             * @param index The index of the color attachment to retrieve. Default is 0.
             * @return The resource ID of the specified color attachment.
             */
            virtual u32 GetColorAttachmentResourceID(u32 index = 0) const = 0;

            /** @brief Retrieves the resource ID of the depth/stencil attachment.
             * @return The resource ID of the depth/stencil attachment.
             */
            virtual u32 GetDepthStencilAttachmentResourceID() const = 0;

            /** @brief Factory method to create a FrameBuffer instance based on the current graphics API.
             * @param specification The specification for the framebuffer to be created.
             * @return A reference-counted pointer to the created FrameBuffer instance.
             */
            static Ref<FrameBuffer> Create(const FrameBufferSpecification &specification);

        protected:
            /** @brief Constructs a FrameBuffer with the given specification.
             * @param specification The specification for the framebuffer.
             */
            FrameBuffer(const FrameBufferSpecification &specification)
                : _specification(specification) {};

            /** @brief The specification of the framebuffer. */
            FrameBufferSpecification _specification;
    };

} // namespace Vulkyrie::Renderer
