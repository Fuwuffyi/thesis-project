#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <span>

class GLBuffer {
  public:
   enum class Type : GLenum {
      Array = GL_ARRAY_BUFFER,
      Element = GL_ELEMENT_ARRAY_BUFFER,
      Uniform = GL_UNIFORM_BUFFER,
      Storage = GL_SHADER_STORAGE_BUFFER,
      TransformFeedback = GL_TRANSFORM_FEEDBACK_BUFFER
   };

   enum class Usage : GLenum {
      StaticDraw = GL_STATIC_DRAW,
      DynamicDraw = GL_DYNAMIC_DRAW,
      StreamDraw = GL_STREAM_DRAW,
      StaticRead = GL_STATIC_READ,
      DynamicRead = GL_DYNAMIC_READ,
      StreamRead = GL_STREAM_READ,
      StaticCopy = GL_STATIC_COPY,
      DynamicCopy = GL_DYNAMIC_COPY,
      StreamCopy = GL_STREAM_COPY
   };

   explicit GLBuffer(const Type type, const Usage usage = Usage::StaticDraw);
   ~GLBuffer();

   GLBuffer(const GLBuffer&) = delete;
   GLBuffer& operator=(const GLBuffer&) = delete;
   GLBuffer(GLBuffer&& other) noexcept;
   GLBuffer& operator=(GLBuffer&& other) noexcept;

   void Bind() const noexcept;
   void BindBase(const uint32_t bindingPoint) const noexcept;
   void Unbind(const Type type) noexcept;

   void UploadData(const void* data, const size_t size);
   void UpdateData(const void* data, const size_t size, const size_t offset = 0) const;

   template <typename T>
   void UploadData(const std::span<const T> data);

   template <typename T>
   void UpdateData(const std::span<const T> data, const size_t offset = 0) const;

   [[nodiscard]] void* Map(const GLenum access = GL_READ_WRITE) const;
   void Unmap() const noexcept;

   [[nodiscard]] constexpr GLuint Get() const noexcept { return m_buffer; }
   [[nodiscard]] constexpr GLBuffer::Type GetType() const noexcept { return m_type; }
   [[nodiscard]] constexpr GLBuffer::Usage GetUsage() const noexcept { return m_usage; }
   [[nodiscard]] constexpr size_t GetSize() const noexcept { return m_size; }
   [[nodiscard]] constexpr bool IsValid() const noexcept { return m_buffer != 0; }

  private:
   GLuint m_buffer{0};
   Type m_type;
   Usage m_usage;
   size_t m_size{0};
};

template <typename T>
void GLBuffer::UploadData(const std::span<const T> data) {
   UploadData(data.data(), data.size_bytes());
}

template <typename T>
void GLBuffer::UpdateData(const std::span<const T> data, const size_t offset) const {
   UpdateData(data.data(), data.size_bytes(), offset);
}
