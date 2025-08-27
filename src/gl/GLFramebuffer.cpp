#include "GLFramebuffer.hpp"
#include "resource/GLTexture.hpp"

#include <stdexcept>
#include <utility>

GLFramebuffer::GLFramebuffer(const CreateInfo& info)
   : m_width(info.width)
   , m_height(info.height)
   , m_colorAttachments(info.colorAttachments)
   , m_depthAttachment(info.depthAttachment)
   , m_stencilAttachment(info.stencilAttachment)
{
   glGenFramebuffers(1, &m_fbo);
   if (m_fbo == 0) {
      throw std::runtime_error("Failed to create OpenGL framebuffer");
   }

   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

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
         drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
      }
      glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
   } else {
      glDrawBuffer(GL_NONE);
      glReadBuffer(GL_NONE);
   }

   // Check completeness
   if (!IsComplete()) {
      const std::string status = GetStatusString();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      throw std::runtime_error("Framebuffer incomplete: " + status);
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLFramebuffer::~GLFramebuffer() {
   if (m_fbo != 0) {
      glDeleteFramebuffers(1, &m_fbo);
   }
}

GLFramebuffer::GLFramebuffer(GLFramebuffer&& other) noexcept
   : m_fbo(std::exchange(other.m_fbo, 0))
   , m_width(other.m_width)
   , m_height(other.m_height)
   , m_colorAttachments(std::move(other.m_colorAttachments))
   , m_depthAttachment(std::move(other.m_depthAttachment))
   , m_stencilAttachment(std::move(other.m_stencilAttachment))
{
}

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

void GLFramebuffer::Bind() const {
   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
   glViewport(0, 0, m_width, m_height);
}

void GLFramebuffer::Unbind() const {
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool GLFramebuffer::IsComplete() const {
   const GLint currentFbo = []() {
      GLint fbo;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      return fbo;
   }();

   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
   const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
   glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
   return complete;
}

std::string GLFramebuffer::GetStatusString() const {
   const GLint currentFbo = []() {
      GLint fbo;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      return fbo;
   }();

   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
   const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);

   switch (status) {
      case GL_FRAMEBUFFER_COMPLETE: return "Complete";
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "Incomplete attachment";
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "Missing attachment";
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "Incomplete draw buffer";
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "Incomplete read buffer";
      case GL_FRAMEBUFFER_UNSUPPORTED: return "Unsupported";
      case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "Incomplete multisample";
      default: return "Unknown error";
   }
}

void GLFramebuffer::Clear(bool clearColor, bool clearDepth, bool clearStencil) const {
   const GLint currentFbo = []() {
      GLint fbo;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      return fbo;
   }();

   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

   GLbitfield mask = 0;
   if (clearColor) mask |= GL_COLOR_BUFFER_BIT;
   if (clearDepth) mask |= GL_DEPTH_BUFFER_BIT;
   if (clearStencil) mask |= GL_STENCIL_BUFFER_BIT;

   glClear(mask);
   glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
}

void GLFramebuffer::ClearColor(uint32_t attachment, float r, float g, float b, float a) const {
   const GLint currentFbo = []() {
      GLint fbo;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      return fbo;
   }();

   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
   const float clearColor[4] = {r, g, b, a};
   glClearBufferfv(GL_COLOR, static_cast<GLint>(attachment), clearColor);
   glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
}

void GLFramebuffer::ClearDepth(float depth) const {
   const GLint currentFbo = []() {
      GLint fbo;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      return fbo;
   }();

   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
   glClearBufferfv(GL_DEPTH, 0, &depth);
   glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
}

void GLFramebuffer::ClearStencil(int32_t stencil) const {
   const GLint currentFbo = []() {
      GLint fbo;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      return fbo;
   }();

   glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
   glClearBufferiv(GL_STENCIL, 0, &stencil);
   glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
}

void GLFramebuffer::BlitTo(const GLFramebuffer& target,
                           uint32_t srcX0, uint32_t srcY0, uint32_t srcX1, uint32_t srcY1,
                           uint32_t dstX0, uint32_t dstY0, uint32_t dstX1, uint32_t dstY1,
                           GLbitfield mask, GLenum filter) const {
   glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.GetId());

   glBlitFramebuffer(
      static_cast<GLint>(srcX0), static_cast<GLint>(srcY0),
      static_cast<GLint>(srcX1), static_cast<GLint>(srcY1),
      static_cast<GLint>(dstX0), static_cast<GLint>(dstY0),
      static_cast<GLint>(dstX1), static_cast<GLint>(dstY1),
      mask, filter
   );
}

void GLFramebuffer::BlitToScreen(uint32_t screenWidth, uint32_t screenHeight,
                                 GLbitfield mask, GLenum filter) const {
   glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBlitFramebuffer(
      0, 0, static_cast<GLint>(m_width), static_cast<GLint>(m_height),
      0, 0, static_cast<GLint>(screenWidth), static_cast<GLint>(screenHeight),
      mask, filter
   );
}

void GLFramebuffer::AttachTexture(const AttachmentDesc& attachment, GLenum attachmentType) {
   if (!attachment.texture) return;

   const GLuint textureId = attachment.texture->GetId();

   // For now, we only support 2D textures. Can be extended for arrays/cubemaps later
   glFramebufferTexture2D(
      GL_FRAMEBUFFER,
      attachmentType,
      GL_TEXTURE_2D,
      textureId,
      static_cast<GLint>(attachment.mipLevel)
   );
}

