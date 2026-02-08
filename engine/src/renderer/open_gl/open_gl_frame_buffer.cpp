#include "renderer/open_gl/open_gl_frame_buffer.h"
#include <glad/glad.h>
#include "core/asserts.h"

namespace Vulkyrie::Renderer {

    static constexpr GLenum ToGLInternalFormat(ColorFormat format) {
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

    static constexpr GLenum ToGLInternalFormat(DepthStencilFormat format) {
        switch (format) {
            case DepthStencilFormat::Depth24Stencil8:
                return GL_DEPTH24_STENCIL8;
            case DepthStencilFormat::Depth32F:
                return GL_DEPTH_COMPONENT32F;
        }

        return GL_DEPTH24_STENCIL8;
    }

    static constexpr GLenum ToGLSamplerWrapMode(TextureSamplerWrapMode mode) {
        switch (mode) {
            case TextureSamplerWrapMode::Repeat:
                return GL_REPEAT;
            case TextureSamplerWrapMode::MirroredRepeat:
                return GL_MIRRORED_REPEAT;
            case TextureSamplerWrapMode::ClampToEdge:
                return GL_CLAMP_TO_EDGE;
            case TextureSamplerWrapMode::ClampToBorder:
                return GL_CLAMP_TO_BORDER;
        }

        return GL_REPEAT;
    }

    static constexpr GLenum ToGLTextureFilterMode(TextureFilterMode mode) {
        switch (mode) {
            case TextureFilterMode::Nearest:
                return GL_NEAREST;
            case TextureFilterMode::Linear:
                return GL_LINEAR;
        }

        return GL_LINEAR;
    }

    static constexpr bool HasStencil(DepthStencilFormat format) {
        return format == DepthStencilFormat::Depth24Stencil8;
    }

    OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferSpecification &specification)
        : FrameBuffer(specification) {
        Create();
    }

    OpenGLFrameBuffer::~OpenGLFrameBuffer() {
        Destroy();
    }

    u32 OpenGLFrameBuffer::GetColorAttachmentResourceID(u32 index) const {
        VASSERT_EXPR(index < _colorAttachments.size(), "Color attachment index out of bounds");
        return _colorAttachments[index].ResourceID;
    }

    u32 OpenGLFrameBuffer::GetDepthStencilAttachmentResourceID() const {
        VASSERT_EXPR(_depthAttachment.ResourceID != 0, "No depth attachment found");
        return _depthAttachment.ResourceID;
    }

    void OpenGLFrameBuffer::Create() {
        Destroy();

        glCreateFramebuffers(1, &_fboId);

        u32 expectedSamples = 1;
        auto checkSamples = [&](u32 samples) {
            if (expectedSamples == 1) {
                expectedSamples = samples;
            } else {
                VASSERT_EXPR(samples == expectedSamples, "All framebuffer attachments must have the same sample count");
            }
        };

        std::vector<GLenum> drawBuffers;

        // Create color attachments if specified.
        if (_specification.ColorAttachments.has_value()) {
            u32 colorIndex = 0;

            _colorAttachments.clear();
            _colorAttachments.reserve(_specification.ColorAttachments->size());
            drawBuffers.reserve(_specification.ColorAttachments->size());

            for (const auto &attachment : *_specification.ColorAttachments) {
                checkSamples(attachment.Samples);

                OpenGLFrameBufferAttachment glAttachment{};
                glAttachment.Type = attachment.Type;
                glAttachment.AttachmentPoint = GL_COLOR_ATTACHMENT0 + colorIndex;

                const bool multisample = attachment.Samples > 1;

                if (attachment.Type == AttachmentType::Texture) {
                    glCreateTextures(multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, 1, &glAttachment.ResourceID);

                    if (multisample) {
                        glTextureStorage2DMultisample(glAttachment.ResourceID,
                                                      attachment.Samples,
                                                      ToGLInternalFormat(attachment.Format),
                                                      _specification.Width,
                                                      _specification.Height,
                                                      GL_TRUE);
                    } else {
                        glTextureStorage2D(glAttachment.ResourceID, 1, ToGLInternalFormat(attachment.Format), _specification.Width, _specification.Height);
                        glTextureParameteri(glAttachment.ResourceID, GL_TEXTURE_MIN_FILTER, ToGLTextureFilterMode(attachment.MinFilter));
                        glTextureParameteri(glAttachment.ResourceID, GL_TEXTURE_MAG_FILTER, ToGLTextureFilterMode(attachment.MagFilter));
                        glTextureParameteri(glAttachment.ResourceID, GL_TEXTURE_WRAP_S, ToGLSamplerWrapMode(attachment.WrapS));
                        glTextureParameteri(glAttachment.ResourceID, GL_TEXTURE_WRAP_T, ToGLSamplerWrapMode(attachment.WrapT));
                    }

                    glNamedFramebufferTexture(_fboId, glAttachment.AttachmentPoint, glAttachment.ResourceID, 0);
                } else {
                    glCreateRenderbuffers(1, &glAttachment.ResourceID);

                    if (multisample) {
                        glNamedRenderbufferStorageMultisample(
                            glAttachment.ResourceID, attachment.Samples, ToGLInternalFormat(attachment.Format), _specification.Width, _specification.Height);
                    } else {
                        glNamedRenderbufferStorage(glAttachment.ResourceID, ToGLInternalFormat(attachment.Format), _specification.Width, _specification.Height);
                    }

                    glNamedFramebufferRenderbuffer(_fboId, glAttachment.AttachmentPoint, GL_RENDERBUFFER, glAttachment.ResourceID);
                }

                _colorAttachments.emplace_back(glAttachment);
                drawBuffers.emplace_back(glAttachment.AttachmentPoint);
                ++colorIndex;
            }
        }

        if (!drawBuffers.empty()) {
            glNamedFramebufferDrawBuffers(_fboId, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
            glNamedFramebufferReadBuffer(_fboId, GL_COLOR_ATTACHMENT0);
        } else {
            glNamedFramebufferDrawBuffer(_fboId, GL_NONE);
            glNamedFramebufferReadBuffer(_fboId, GL_NONE);
        }

        // Create depth/stencil attachment if specified.
        if (_specification.DepthStencilAttachment.has_value()) {
            const auto &depth = *_specification.DepthStencilAttachment;

            checkSamples(depth.Samples);

            const bool multisample = depth.Samples > 1;
            const bool hasStencil = HasStencil(depth.Format);

            _depthAttachment = {};
            _depthAttachment.Type = depth.Type;
            _depthAttachment.AttachmentPoint = hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

            if (depth.Type == AttachmentType::Texture) {
                glCreateTextures(multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, 1, &_depthAttachment.ResourceID);

                if (multisample) {
                    glTextureStorage2DMultisample(
                        _depthAttachment.ResourceID, depth.Samples, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height, GL_TRUE);
                } else {
                    glTextureStorage2D(_depthAttachment.ResourceID, 1, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height);
                    glTextureParameteri(_depthAttachment.ResourceID, GL_TEXTURE_MIN_FILTER, ToGLTextureFilterMode(depth.MinFilter));
                    glTextureParameteri(_depthAttachment.ResourceID, GL_TEXTURE_MAG_FILTER, ToGLTextureFilterMode(depth.MagFilter));
                    glTextureParameteri(_depthAttachment.ResourceID, GL_TEXTURE_WRAP_S, ToGLSamplerWrapMode(depth.WrapS));
                    glTextureParameteri(_depthAttachment.ResourceID, GL_TEXTURE_WRAP_T, ToGLSamplerWrapMode(depth.WrapT));
                }

                glNamedFramebufferTexture(_fboId, _depthAttachment.AttachmentPoint, _depthAttachment.ResourceID, 0);
            } else {
                glCreateRenderbuffers(1, &_depthAttachment.ResourceID);

                if (multisample) {
                    glNamedRenderbufferStorageMultisample(
                        _depthAttachment.ResourceID, depth.Samples, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height);
                } else {
                    glNamedRenderbufferStorage(_depthAttachment.ResourceID, ToGLInternalFormat(depth.Format), _specification.Width, _specification.Height);
                }

                glNamedFramebufferRenderbuffer(_fboId, _depthAttachment.AttachmentPoint, GL_RENDERBUFFER, _depthAttachment.ResourceID);
            }
        }

        // Validate framebuffer completeness.
        GLenum status = glCheckNamedFramebufferStatus(_fboId, GL_FRAMEBUFFER);
        VASSERT_EXPR(GL_FRAMEBUFFER_COMPLETE == status, "Framebuffer is incomplete (status = {})", status);
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

        // Clear color attachments if needed.
        if (_specification.ColorAttachments.has_value()) {
            for (size_t i = 0; i < _colorAttachments.size(); ++i) {
                const auto &spec = (*_specification.ColorAttachments)[i];

                if (spec.Load != LoadOp::Clear) continue;

                const auto &glAtt = _colorAttachments[i];
                GLint colorIndex = static_cast<GLint>(glAtt.AttachmentPoint - GL_COLOR_ATTACHMENT0);

                if (spec.Format == ColorFormat::R32I) {
                    const GLint value[4] = { static_cast<GLint>(spec.ClearColor[0]), 0, 0, 0 };
                    glClearNamedFramebufferiv(_fboId, GL_COLOR, colorIndex, value);
                } else {
                    glClearNamedFramebufferfv(_fboId, GL_COLOR, colorIndex, spec.ClearColor);
                }
            }
        }

        // Clear depth/stencil attachment if needed.
        if (_specification.DepthStencilAttachment.has_value()) {
            const auto &depth = *_specification.DepthStencilAttachment;

            if (depth.Load == LoadOp::Clear) {
                if (HasStencil(depth.Format)) {
                    glClearNamedFramebufferfi(_fboId, GL_DEPTH_STENCIL, 0, depth.ClearDepth, depth.ClearStencil);
                } else {
                    glClearNamedFramebufferfv(_fboId, GL_DEPTH, 0, &depth.ClearDepth);
                }
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

        for (auto &att : _colorAttachments) {
            if (att.ResourceID == 0) continue;

            if (att.Type == AttachmentType::Texture) {
                glDeleteTextures(1, &att.ResourceID);
            } else {
                glDeleteRenderbuffers(1, &att.ResourceID);
            }
        }
        _colorAttachments.clear();

        if (_depthAttachment.ResourceID) {
            if (_depthAttachment.Type == AttachmentType::Texture) {
                glDeleteTextures(1, &_depthAttachment.ResourceID);
            } else {
                glDeleteRenderbuffers(1, &_depthAttachment.ResourceID);
            }

            _depthAttachment = {};
        }
    }

} // namespace Vulkyrie::Renderer
