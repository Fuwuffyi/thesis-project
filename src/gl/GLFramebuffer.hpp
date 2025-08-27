#pragma once

#include <glad/gl.h>
#include <vector>
#include <memory>
#include <cstdint>

class GLTexture;
class ITexture;

class GLFramebuffer {
public:
   struct AttachmentDesc {
      GLTexture* texture;
      uint32_t mipLevel = 0;
      uint32_t layer = 0;
   };

   struct CreateInfo {
      std::vector<AttachmentDesc> colorAttachments;
      AttachmentDesc depthAttachment;
      AttachmentDesc stencilAttachment;
      uint32_t width = 0;
      uint32_t height = 0;
   };

   GLFramebuffer(const CreateInfo& info);
   ~GLFramebuffer();

   GLFramebuffer(const GLFramebuffer&) = delete;
   GLFramebuffer& operator=(const GLFramebuffer&) = delete;
   GLFramebuffer(GLFramebuffer&& other) noexcept;
   GLFramebuffer& operator=(GLFramebuffer&& other) noexcept;

   void Bind() const;
   void Unbind() const;

   // Check if framebuffer is complete
   bool IsComplete() const;
   std::string GetStatusString() const;

   // Clear operations
   void Clear(bool clearColor = true, bool clearDepth = true, bool clearStencil = false) const;
   void ClearColor(uint32_t attachment, float r, float g, float b, float a) const;
   void ClearDepth(float depth) const;
   void ClearStencil(int32_t stencil) const;

   // Getters
   GLuint GetId() const { return m_fbo; }
   uint32_t GetWidth() const { return m_width; }
   uint32_t GetHeight() const { return m_height; }
   size_t GetColorAttachmentCount() const { return m_colorAttachments.size(); }
   bool HasDepthAttachment() const { return m_depthAttachment.texture != nullptr; }
   bool HasStencilAttachment() const { return m_stencilAttachment.texture != nullptr; }

   // Blit operations
   void BlitTo(const GLFramebuffer& target, 
               uint32_t srcX0, uint32_t srcY0, uint32_t srcX1, uint32_t srcY1,
               uint32_t dstX0, uint32_t dstY0, uint32_t dstX1, uint32_t dstY1,
               GLbitfield mask = GL_COLOR_BUFFER_BIT, 
               GLenum filter = GL_LINEAR) const;

   void BlitToScreen(uint32_t screenWidth, uint32_t screenHeight,
                     GLbitfield mask = GL_COLOR_BUFFER_BIT,
                     GLenum filter = GL_LINEAR) const;

private:
   void AttachTexture(const AttachmentDesc& attachment, GLenum attachmentType);

private:
   GLuint m_fbo = 0;
   uint32_t m_width = 0;
   uint32_t m_height = 0;

   std::vector<AttachmentDesc> m_colorAttachments;
   AttachmentDesc m_depthAttachment;
   AttachmentDesc m_stencilAttachment;
};

