#include "gl/GLFramebuffer.hpp"

#include "gl/resource/GLTexture.hpp"

#include <utility>
#include <format>

using namespace std::string_view_literals;

GLFramebuffer::GLFramebuffer(const CreateInfo& info)
    : m_width(info.width),
      m_height(info.height),
      m_colorAttachments(info.colorAttachments),
      m_depthAttachment(info.depthAttachment),
      m_stencilAttachment(info.stencilAttachment) {
   glGenFramebuffers(1, &m_fbo);
   if (m_fbo == 0) [[unlikely]] {
      throw std::runtime_error("Failed to create OpenGL framebuffer");
   }
   const ScopedBinder binder(m_fbo);
   // Attach color attachments
   for (size_t i = 0; i < m_colorAttachments.size(); ++i) {
      AttachTexture(m_colorAttachments[i], GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
   }
   // Attach depth attachment
   if (m_depthAttachment.texture) {
      AttachTexture(m_depthAttachment, GL_DEPTH_ATTACHMENT);
   }
   // Attach stencil attachment
   if (m_stencilAttachment.texture) {
      AttachTexture(m_stencilAttachment, GL_STENCIL_ATTACHMENT);
   }
   // Set draw buffers
   if (!m_colorAttachments.empty()) {
      std::vector<GLenum> drawBuffers;
      drawBuffers.reserve(m_colorAttachments.size());
      for (size_t i = 0; i < m_colorAttachments.size(); ++i) {
         drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
      }
      glDrawBuffers(drawBuffers.size(), drawBuffers.data());
   } else {
      glDrawBuffer(GL_NONE);
      glReadBuffer(GL_NONE);
   }
   // Check completeness
   if (!IsComplete()) [[unlikely]] {
      throw std::runtime_error(std::format("Framebuffer incomplete: {}", GetStatusString()));
   }
}

GLFramebuffer::~GLFramebuffer() {
   if (m_fbo != 0) {
      glDeleteFramebuffers(1, &m_fbo);
   }
}

GLFramebuffer::GLFramebuffer(GLFramebuffer&& other) noexcept
    : m_fbo(std::exchange(other.m_fbo, 0)),
      m_width(other.m_width),
      m_height(other.m_height),
      m_colorAttachments(std::move(other.m_colorAttachments)),
      m_depthAttachment(std::move(other.m_depthAttachment)),
      m_stencilAttachment(std::move(other.m_stencilAttachment)) {}

GLFramebuffer& GLFramebuffer::operator=(GLFramebuffer&& other) noexcept {
   if (this != &other) {
      if (m_fbo != 0) {
         glDeleteFramebuffers(1, &m_fbo);
      }
      m_fbo = std::exchange(other.m_fbo, 0);
      m_width = other.m_width;
      m_height = other.m_height;
      m_colorAttachments = std::move(other.m_colorAttachments);
      m_depthAttachment = std::move(other.m_depthAttachment);
      m_stencilAttachment = std::move(other.m_stencilAttachment);
   }
   return *this;
}

void GLFramebuffer::Bind() const noexcept {
   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
   glViewport(0, 0, m_width, m_height);
}

void GLFramebuffer::Unbind() const noexcept { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

bool GLFramebuffer::IsComplete() const noexcept { return GetStatus() == Status::Complete; }

GLFramebuffer::Status GLFramebuffer::GetStatus() const noexcept {
   const ScopedBinder binder(m_fbo);
   const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   switch (status) {
      case GL_FRAMEBUFFER_COMPLETE:
         return Status::Complete;
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
         return Status::IncompleteAttachment;
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
         return Status::MissingAttachment;
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
         return Status::IncompleteDrawBuffer;
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
         return Status::IncompleteReadBuffer;
      case GL_FRAMEBUFFER_UNSUPPORTED:
         return Status::Unsupported;
      case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
         return Status::IncompleteMultisample;
      default:
         return Status::Unknown;
   }
}

std::string GLFramebuffer::GetStatusString() const noexcept {
   switch (GetStatus()) {
      case Status::Complete:
         return "Complete";
      case Status::IncompleteAttachment:
         return "Incomplete attachment";
      case Status::MissingAttachment:
         return "Missing attachment";
      case Status::IncompleteDrawBuffer:
         return "Incomplete draw buffer";
      case Status::IncompleteReadBuffer:
         return "Incomplete read buffer";
      case Status::Unsupported:
         return "Unsupported";
      case Status::IncompleteMultisample:
         return "Incomplete multisample";
      default:
         return "Unknown error";
   }
}

void GLFramebuffer::Clear(const bool clearColor, const bool clearDepth,
                          const bool clearStencil) const noexcept {
   const ScopedBinder binder(m_fbo);
   GLbitfield mask = 0;
   if (clearColor)
      mask |= GL_COLOR_BUFFER_BIT;
   if (clearDepth)
      mask |= GL_DEPTH_BUFFER_BIT;
   if (clearStencil)
      mask |= GL_STENCIL_BUFFER_BIT;
   glClear(mask);
}

void GLFramebuffer::ClearColor(const uint32_t attachment, const float r, const float g,
                               const float b, const float a) const noexcept {
   const ScopedBinder binder(m_fbo);
   const float clearColor[4] = {r, g, b, a};
   glClearBufferfv(GL_COLOR, static_cast<GLint>(attachment), clearColor);
}

void GLFramebuffer::ClearDepth(const float depth) const noexcept {
   const ScopedBinder binder(m_fbo);
   glClearBufferfv(GL_DEPTH, 0, &depth);
}

void GLFramebuffer::ClearStencil(const int32_t stencil) const noexcept {
   const ScopedBinder binder(m_fbo);
   glClearBufferiv(GL_STENCIL, 0, &stencil);
}

void GLFramebuffer::BlitTo(const GLFramebuffer& target, const uint32_t srcX0, const uint32_t srcY0,
                           const uint32_t srcX1, const uint32_t srcY1, const uint32_t dstX0,
                           const uint32_t dstY0, const uint32_t dstX1, const uint32_t dstY1,
                           const GLbitfield mask, const GLenum filter) const noexcept {
   glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.GetId());
   glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void GLFramebuffer::BlitToScreen(const uint32_t screenWidth, const uint32_t screenHeight,
                                 const GLbitfield mask, const GLenum filter) const noexcept {
   glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, screenWidth, screenHeight, mask, filter);
}

void GLFramebuffer::AttachTexture(const AttachmentDesc& attachment,
                                  const uint32_t attachmentType) noexcept {
   if (!attachment.texture)
      return;
   // TODO: Add support for other texture types (3D, cube maps, etc.)
   glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D,
                          attachment.texture->GetId(), attachment.mipLevel);
}

GLFramebuffer::ScopedBinder::ScopedBinder(const uint32_t fbo) noexcept {
   glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_previousFbo);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

GLFramebuffer::ScopedBinder::~ScopedBinder() { glBindFramebuffer(GL_FRAMEBUFFER, m_previousFbo); }
