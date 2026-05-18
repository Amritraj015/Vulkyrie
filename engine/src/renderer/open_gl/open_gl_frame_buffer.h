#pragma once

#include "renderer/frame_buffer.h"

namespace Vulkyrie {

    /** @brief Structure representing an OpenGL attachment (color or depth). */
    struct OpenGLFrameBufferAttachment {
    public:
        /** @brief The attachment point for the framebuffer (e.g., GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT). */
        u32 AttachmentPoint = 0;

        /** @brief The OpenGL resource ID of the attachment. */
        u32 ResourceID = 0;

        /** @brief The type of the attachment (Texture or RenderBuffer). */
        AttachmentType Type = AttachmentType::Texture;
    };

    /** @brief OpenGL implementation of a Frame Buffer. */
    class OpenGLFrameBuffer final : public FrameBuffer {
    public:
        /** @brief Constructs an OpenGL Frame Buffer with the given specification.
         * @param specification The specification for the framebuffer.
         */
        OpenGLFrameBuffer(const FrameBufferSpecification &specification);

        /** @brief Destructor that cleans up the framebuffer resources. */
        ~OpenGLFrameBuffer() override;

        /** @brief Resizes the framebuffer to the specified width and height.
         * @param width The new width of the framebuffer.
         * @param height The new height of the framebuffer.
         */
        void Resize(u32 width, u32 height) override;

        /** @brief Binds the framebuffer for rendering. */
        void Bind() const override;

        /** @brief Unbinds the framebuffer, reverting to the default framebuffer. */
        void Unbind() const override;

        /** @brief Retrieves the resource ID of the specified color attachment.
         * @param index The index of the color attachment to retrieve. Default is 0.
         * @returns The resource ID of the specified color attachment.
         */
        [[nodiscard]] u32 GetColorAttachmentResourceID(u32 index = 0) const override;

        /** @brief Retrieves the resource ID of the depth/stencil attachment.
         * @returns The resource ID of the depth/stencil attachment.
         */
        [[nodiscard]] u32 GetDepthStencilAttachmentResourceID() const override;

        /** @brief Retrieves the resource ID of the framebuffer itself (i.e., FBO ID in OpenGL).
         * @returns The resource ID of the framebuffer.
         */
        [[nodiscard]] inline u32 GetFrameBufferID() const override {
            return _fboId;
        }

    private:
        /** @brief The OpenGL Frame Buffer Object ID. */
        u32 _fboId;

        /** @brief Color attachments associated with the framebuffer. */
        std::vector<OpenGLFrameBufferAttachment> _colorAttachments;

        /** @brief Depth attachment associated with the framebuffer. */
        OpenGLFrameBufferAttachment _depthAttachment;

        /** @brief Creates the framebuffer and its attachments based on the specification. */
        void Create();

        /** @brief Destroys the framebuffer and releases its resources. */
        void Destroy();
    };

} // namespace Vulkyrie
