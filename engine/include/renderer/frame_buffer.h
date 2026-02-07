#pragma once

namespace Vulkyrie::Renderer {
    enum class AttachmentType : u8 { Texture, RenderBuffer };
    enum class ColorFormat : u8 { RGBA8, RGBA16F, RGBA32F, R32I };
    enum class DepthStencilFormat : u8 { Depth24Stencil8, Depth32F };

    enum class LoadOp {
        Load,  // Preserve previous contents
        Clear, // Clear at pass start
        DontCare
    };

    enum class StoreOp {
        Store, // Keep results
        DontCare
    };

    /** @brief Specification structure for color attachment. */
    struct ColorAttachmentSpecification {
        public:
            /** @brief Format of the color attachment. */
            ColorFormat Format;

            /** @brief Type of the attachment (Texture or RenderBuffer). */
            AttachmentType Type = AttachmentType::Texture;

            /** @brief Load operation for the color attachment. */
            LoadOp Load = LoadOp::Clear;

            /** @brief Store operation for the color attachment. */
            StoreOp Store = StoreOp::Store;

            /** @brief Number of samples for multi-sampling. */
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
            AttachmentType Type = AttachmentType::RenderBuffer;

            /** @brief Load operation for the depth attachment. */
            LoadOp Load = LoadOp::Clear;

            /** @brief Store operation for the depth attachment. */
            StoreOp Store = StoreOp::Store;

            /** @brief Number of samples for multi-sampling. */
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
            std::vector<ColorAttachmentSpecification> ColorAttachments;

            /** @brief Specification for the depth and stencil attachments. */
            std::optional<DepthStencilAttachmentSpecification> DepthStencilAttachment;

            /** @brief Indicates if the framebuffer is a swapchain target. */
            bool SwapchainTarget = false;

            /** @brief Optional debug name for the framebuffer. */
            const char *DebugName = nullptr;
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
