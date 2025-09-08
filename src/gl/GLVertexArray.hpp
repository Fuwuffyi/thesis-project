#pragma once

#include <cstddef>
#include <cstdint>

class GLBuffer;

class GLVertexArray final {
  public:
   GLVertexArray();
   ~GLVertexArray();

   GLVertexArray(const GLVertexArray&) = delete;
   GLVertexArray& operator=(const GLVertexArray&) = delete;
   GLVertexArray(GLVertexArray&& other) noexcept;
   GLVertexArray& operator=(GLVertexArray&& other) noexcept;

   void Bind() const noexcept;
   static void Unbind() noexcept;

   void AttachVertexBuffer(const GLBuffer& buffer, const uint32_t bindingIndex = 0,
                           const size_t offset = 0, const size_t stride = 0) const;
   void AttachElementBuffer(const GLBuffer& buffer) const;

   void SetupVertexAttributes() const;
   void EnableAttribute(const uint32_t index) const noexcept;
   void DisableAttribute(const uint32_t index) const noexcept;
   void SetAttributeFormat(const uint32_t index, const size_t size, const uint32_t type,
                           bool normalized, const uint32_t relativeOffset) const noexcept;
   void SetAttributeBinding(const uint32_t index, const uint32_t bindingIndex) const noexcept;

   void DrawArrays(const uint32_t mode, const uint32_t first, const size_t count) const noexcept;
   void DrawElements(const uint32_t mode, const size_t count, const uint32_t type,
                     const void* indices = nullptr) const noexcept;
   void DrawElementsInstanced(const uint32_t mode, const size_t count, const uint32_t type,
                              const void* indices, const size_t instanceCount) const noexcept;

   [[nodiscard]] constexpr uint32_t Get() const noexcept { return m_vao; }
   [[nodiscard]] constexpr bool IsValid() const noexcept { return m_vao != 0; }

  private:
   uint32_t m_vao{0};
};
