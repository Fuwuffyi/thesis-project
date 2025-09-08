#pragma once

#include <cstddef>
#include <cstdint>

class GLBuffer;

class GLVertexArray {
  public:
   GLVertexArray();
   ~GLVertexArray();

   GLVertexArray(const GLVertexArray&) = delete;
   GLVertexArray& operator=(const GLVertexArray&) = delete;
   GLVertexArray(GLVertexArray&& other) noexcept;
   GLVertexArray& operator=(GLVertexArray&& other) noexcept;

   void Bind() const noexcept;
   static void Unbind() noexcept;

   void AttachVertexBuffer(const GLBuffer& buffer, uint32_t bindingIndex = 0, size_t offset = 0,
                           size_t stride = 0) const;
   void AttachElementBuffer(const GLBuffer& buffer) const;

   void SetupVertexAttributes() const;
   void EnableAttribute(uint32_t index) const noexcept;
   void DisableAttribute(uint32_t index) const noexcept;
   void SetAttributeFormat(uint32_t index, int32_t size, uint32_t type, bool normalized,
                           uint32_t relativeOffset) const noexcept;
   void SetAttributeBinding(uint32_t index, uint32_t bindingIndex) const noexcept;

   void DrawArrays(uint32_t mode, int32_t first, size_t count) const noexcept;
   void DrawElements(uint32_t mode, size_t count, uint32_t type,
                     const void* indices = nullptr) const noexcept;
   void DrawElementsInstanced(uint32_t mode, size_t count, uint32_t type, const void* indices,
                              size_t instanceCount) const noexcept;

   [[nodiscard]] constexpr uint32_t Get() const noexcept { return m_vao; }
   [[nodiscard]] constexpr bool IsValid() const noexcept { return m_vao != 0; }

  private:
   uint32_t m_vao{0};
};
