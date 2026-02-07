#include "renderer/open_gl/open_gl_frame_buffer.h"
#include <glad/glad.h>
#include "core/asserts.h"

namespace Vulkyrie::Renderer {

    static GLenum ToGLInternalFormat(ColorFormat format) {
        switch (format) {
            case ColorFormat::RGBA8:
                return GL_RGBA8;
            case ColorFormat::RGBA16F:
                return GL_RGBA16F;
            case ColorFormat::RGBA32F:
                return GL_RGBA32F;
            case ColorFormat::R32I:
                return GL_R32I;
        }

        return GL_RGBA8;
    }

    static GLenum ToGLInternalFormat(DepthStencilFormat format) {
        switch (format) {
            case DepthStencilFormat::Depth24Stencil8:
                return GL_DEPTH24_STENCIL8;
            case DepthStencilFormat::Depth32F:
                return GL_DEPTH_COMPONENT32F;
        }

        return GL_DEPTH24_STENCIL8;
    }

    OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferSpecification &specification)
        : FrameBuffer(specification) {
        Create();
    }

    u32 OpenGLFrameBuffer::GetColorAttachmentResourceID(u32 index) const {
        VASSERT_EXPR(index < _colorAttachments.size(), "Color attachment index out of bounds!");
        return _colorAttachments[index].ResourceID;
    }

    void OpenGLFrameBuffer::Create() {
        Destroy();

        glCreateFramebuffers(1, &_fboId);

        u32 colorIndex = 0;
        std::vector<GLenum> drawBuffers;
        drawBuffers.reserve(_specification.ColorAttachments.size());
        _colorAttachments.reserve(_specification.ColorAttachments.size());

        // --- Color attachments ---
        for (const auto &attachment : _specification.ColorAttachments) {
            const bool multiSample = attachment.Samples > 1;
            OpenGLFrameBufferAttachment colorAttachment;
            colorAttachment.Type = attachment.Type;

            if (attachment.Type == AttachmentType::Texture) {
                glCreateTextures(multiSample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, 1, &colorAttachment.ResourceID);

                if (multiSample) {
                    glTextureStorage2DMultisample(colorAttachment.ResourceID,
                                                  attachment.Samples,
                                                  ToGLInternalFormat(attachment.Format),
                                                  _specification.Width,
                                                  _specification.Height,
                                                  GL_TRUE);
                } else {
                    glTextureStorage2D(colorAttachment.ResourceID, 1, ToGLInternalFormat(attachment.Format), _specification.Width, _specification.Height);

                    glTextureParameteri(colorAttachment.ResourceID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTextureParameteri(colorAttachment.ResourceID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTextureParameteri(colorAttachment.ResourceID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTextureParameteri(colorAttachment.ResourceID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }

                glNamedFramebufferTexture(_fboId, GL_COLOR_ATTACHMENT0 + colorIndex, colorAttachment.ResourceID, 0);
            } else {
                glCreateRenderbuffers(1, &colorAttachment.ResourceID);

                if (multiSample) {
                    glNamedRenderbufferStorageMultisample(
                        colorAttachment.ResourceID, attachment.Samples, ToGLInternalFormat(attachment.Format), _specification.Width, _specification.Height);
                } else {
                    glNamedRenderbufferStorage(colorAttachment.ResourceID, ToGLInternalFormat(attachment.Format), _specification.Width, _specification.Height);
                }

                glNamedFramebufferRenderbuffer(_fboId, GL_COLOR_ATTACHMENT0 + colorIndex, GL_RENDERBUFFER, colorAttachment.ResourceID);
            }

            _colorAttachments.emplace_back(colorAttachment);
            drawBuffers.emplace_back(GL_COLOR_ATTACHMENT0 + colorIndex);
            colorIndex++;
        }

        VASSERT_EXPR(_colorAttachments.size() == _specification.ColorAttachments.size(), "Color attachment count mismatch!");

        if (!drawBuffers.empty()) {
            glNamedFramebufferDrawBuffers(_fboId, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
        } else {
            glNamedFramebufferDrawBuffers(_fboId, 0, nullptr);
        }

        // --- Depth attachment ---
        if (_specification.DepthStencilAttachment.has_value()) {
            const auto &depth = *_specification.DepthStencilAttachment;
            const bool multisample = depth.Samples > 1;
            _depthAttachment.Type = depth.Type;

            if (depth.Type == AttachmentType::Texture) {
                glCreateTextures(multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, 1, &_depthAttachment.ResourceID);

                if (multisample) {
                    glTextureStorage2DMultisample(
                        _depthAttachment.ResourceID, depth.Samples, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height, GL_TRUE);
                } else {
                    glTextureStorage2D(_depthAttachment.ResourceID, 1, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height);
                }

                // TODO: DO NOT hard code the target to DEPTH_STENCIL_ATTACHMENT, make this configurable.
                glNamedFramebufferTexture(_fboId, GL_DEPTH_STENCIL_ATTACHMENT, _depthAttachment.ResourceID, 0);
            } else {
                glCreateRenderbuffers(1, &_depthAttachment.ResourceID);

                if (multisample) {
                    glNamedRenderbufferStorageMultisample(
                        _depthAttachment.ResourceID, depth.Samples, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height);
                } else {
                    glNamedRenderbufferStorage(_depthAttachment.ResourceID, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height);
                }

                // TODO: DO NOT hard code the target to DEPTH_STENCIL_ATTACHMENT, make this configurable.
                glNamedFramebufferRenderbuffer(_fboId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depthAttachment.ResourceID);
            }
        }

        // Check completeness
        GLenum status = glCheckNamedFramebufferStatus(_fboId, GL_FRAMEBUFFER);

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            VERROR("Framebuffer is incomplete! Status: {0}", status);
        }
    }

    void OpenGLFrameBuffer::Resize(u32 width, u32 height) {
        if (width == 0 || height == 0) return;

        _specification.Width = width;
        _specification.Height = height;

        Create();
    }

    void OpenGLFrameBuffer::Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
        glViewport(0, 0, _specification.Width, _specification.Height);

        // Clear color attachments
        for (size_t i = 0; i < _specification.ColorAttachments.size(); ++i) {
            const auto &attachment = _specification.ColorAttachments[i];

            if (attachment.Load == LoadOp::Clear) {
                glClearNamedFramebufferfv(_fboId, GL_COLOR, static_cast<GLint>(i), attachment.ClearColor);
            }
        }

        // Clear depth/stencil
        if (_specification.DepthStencilAttachment.has_value()) {
            const auto &depth = *_specification.DepthStencilAttachment;

            if (depth.Load == LoadOp::Clear) {
                glClearNamedFramebufferfi(_fboId, GL_DEPTH_STENCIL, 0, depth.ClearDepth, depth.ClearStencil);
            }
        }
    }

    void OpenGLFrameBuffer::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFrameBuffer::Destroy() {
        if (_fboId) {
            glDeleteFramebuffers(1, &_fboId);
            _fboId = 0;
        }

        // Delete all color attachments
        for (auto &att : _colorAttachments) {
            if (att.Type == AttachmentType::Texture) {
                glDeleteTextures(1, &att.ResourceID);
            } else {
                glDeleteRenderbuffers(1, &att.ResourceID);
            }
        }
        _colorAttachments.clear();

        // Delete depth attachment if present
        if (_specification.DepthStencilAttachment.has_value() && _depthAttachment.ResourceID) {
            if (_specification.DepthStencilAttachment->Type == AttachmentType::Texture) {
                glDeleteTextures(1, &_depthAttachment.ResourceID);
            } else {
                glDeleteRenderbuffers(1, &_depthAttachment.ResourceID);
            }

            _depthAttachment = {};
        }
    }

    OpenGLFrameBuffer::~OpenGLFrameBuffer() {
        Destroy();
    }

} // namespace Vulkyrie::Renderer
