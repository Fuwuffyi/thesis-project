#pragma once

#include <glad/gl.h>

#include <string>
#include <vector>
#include <cstdint>

class GLTexture;

class GLFramebuffer final {
  public:
   struct AttachmentDesc final {
      GLTexture* texture{nullptr};
      uint32_t mipLevel{0};
      uint32_t layer{0};

      constexpr AttachmentDesc() noexcept = default;
      constexpr AttachmentDesc(GLTexture* tex, const uint32_t mip = 0,
                               const uint32_t lyr = 0) noexcept
          : texture(tex), mipLevel(mip), layer(lyr) {}
   };

   struct CreateInfo final {
      std::vector<AttachmentDesc> colorAttachments;
      AttachmentDesc depthAttachment{};
      AttachmentDesc stencilAttachment{};
      uint32_t width{0};
      uint32_t height{0};
   };

   enum class Status : uint32_t {
      Complete = GL_FRAMEBUFFER_COMPLETE,
      IncompleteAttachment = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,
      MissingAttachment = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,
      IncompleteDrawBuffer = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,
      IncompleteReadBuffer = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,
      Unsupported = GL_FRAMEBUFFER_UNSUPPORTED,
      IncompleteMultisample = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
      Unknown = 0
   };

   explicit GLFramebuffer(const CreateInfo& info);
   ~GLFramebuffer();

   // Non-copyable, movable
   GLFramebuffer(const GLFramebuffer&) = delete;
   GLFramebuffer& operator=(const GLFramebuffer&) = delete;
   GLFramebuffer(GLFramebuffer&& other) noexcept;
   GLFramebuffer& operator=(GLFramebuffer&& other) noexcept;

   void Bind() const noexcept;
   void Unbind() const noexcept;

   [[nodiscard]] bool IsComplete() const noexcept;
   [[nodiscard]] Status GetStatus() const noexcept;
   [[nodiscard]] std::string GetStatusString() const noexcept;

   // Clear operations with better defaults
   void Clear(const bool clearColor = true, const bool clearDepth = true,
              const bool clearStencil = false) const noexcept;
   void ClearColor(const uint32_t attachment, const float r, const float g, const float b,
                   const float a = 1.0f) const noexcept;
   void ClearDepth(const float depth = 1.0f) const noexcept;
   void ClearStencil(const int32_t stencil = 0) const noexcept;

   // Getters
   [[nodiscard]] constexpr uint32_t GetId() const noexcept { return m_fbo; }
   [[nodiscard]] constexpr uint32_t GetWidth() const noexcept { return m_width; }
   [[nodiscard]] constexpr uint32_t GetHeight() const noexcept { return m_height; }
   [[nodiscard]] constexpr std::size_t GetColorAttachmentCount() const noexcept {
      return m_colorAttachments.size();
   }
   [[nodiscard]] constexpr bool HasDepthAttachment() const noexcept {
      return m_depthAttachment.texture != nullptr;
   }
   [[nodiscard]] constexpr bool HasStencilAttachment() const noexcept {
      return m_stencilAttachment.texture != nullptr;
   }

   // Blit operations with better parameter organization
   void BlitTo(const GLFramebuffer& target, const uint32_t srcX0, const uint32_t srcY0,
               const uint32_t srcX1, const uint32_t srcY1, const uint32_t dstX0,
               const uint32_t dstY0, const uint32_t dstX1, const uint32_t dstY1,
               const GLbitfield mask = GL_COLOR_BUFFER_BIT,
               const uint32_t filter = GL_LINEAR) const noexcept;

   void BlitToScreen(const uint32_t screenWidth, const uint32_t screenHeight,
                     const GLbitfield mask = GL_COLOR_BUFFER_BIT,
                     const uint32_t filter = GL_LINEAR) const noexcept;

  private:
   void AttachTexture(const AttachmentDesc& attachment, const uint32_t attachmentType) noexcept;

   class ScopedBinder final {
     public:
      explicit ScopedBinder(const uint32_t fbo) noexcept;
      ~ScopedBinder();
      ScopedBinder(const ScopedBinder&) = delete;
      ScopedBinder& operator=(const ScopedBinder&) = delete;
      ScopedBinder(ScopedBinder&&) = delete;
      ScopedBinder& operator=(ScopedBinder&&) = delete;

     private:
      int32_t m_previousFbo;
   };

   uint32_t m_fbo{0};
   uint32_t m_width{0};
   uint32_t m_height{0};
   std::vector<AttachmentDesc> m_colorAttachments;
   AttachmentDesc m_depthAttachment{};
   AttachmentDesc m_stencilAttachment{};
};
