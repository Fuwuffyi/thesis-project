#pragma once

#include <glad/gl.h>

#include <string_view>
#include <vector>
#include <cstdint>
#include <span>

class GLTexture;

class GLFramebuffer final {
  public:
   struct AttachmentDesc final {
      const GLTexture* texture{nullptr};
      uint32_t mipLevel{0};
      uint32_t layer{0};

      constexpr AttachmentDesc() noexcept = default;
      constexpr AttachmentDesc(const GLTexture* tex, const uint32_t mip = 0,
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

   [[nodiscard]] static GLFramebuffer Create(const CreateInfo& info) noexcept;

   explicit GLFramebuffer(const CreateInfo& info);
   ~GLFramebuffer() noexcept;

   // Non-copyable, movable
   GLFramebuffer(const GLFramebuffer&) = delete;
   GLFramebuffer& operator=(const GLFramebuffer&) = delete;
   GLFramebuffer(GLFramebuffer&& other) noexcept;
   GLFramebuffer& operator=(GLFramebuffer&& other) noexcept;

   void Bind() const noexcept;
   static void Unbind() noexcept;

   [[nodiscard]] constexpr bool IsComplete() const noexcept { return m_status == Status::Complete; }
   [[nodiscard]] constexpr Status GetStatus() const noexcept { return m_status; }
   [[nodiscard]] std::string_view GetStatusString() const noexcept;

   // Clear operations
   void Clear(const bool clearColor = true, const bool clearDepth = true,
              const bool clearStencil = false) const noexcept;
   void ClearColor(const uint32_t attachment, const std::span<const float, 4> color) const noexcept;
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
   [[nodiscard]] constexpr bool IsValid() const noexcept { return m_fbo != 0; }

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
   void AttachTexture(const AttachmentDesc& attachment,
                      const uint32_t attachmentType) const noexcept;
   [[nodiscard]] Status CheckStatus() const noexcept;

   class ScopedBinder final {
     public:
      explicit ScopedBinder(const uint32_t fbo) noexcept;
      ~ScopedBinder() noexcept;

      ScopedBinder(const ScopedBinder&) = delete;
      ScopedBinder& operator=(const ScopedBinder&) = delete;
      ScopedBinder(ScopedBinder&&) = delete;
      ScopedBinder& operator=(ScopedBinder&&) = delete;

     private:
      uint32_t m_previousFbo;
   };

   uint32_t m_fbo{0};
   uint32_t m_width{0};
   uint32_t m_height{0};
   Status m_status{Status::Unknown};
   std::vector<AttachmentDesc> m_colorAttachments;
   AttachmentDesc m_depthAttachment{};
   AttachmentDesc m_stencilAttachment{};
};
