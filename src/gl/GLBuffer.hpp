#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <vector>

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

   GLBuffer(const Type type, const Usage usage = Usage::StaticDraw);
   ~GLBuffer();

   GLBuffer(const GLBuffer&) = delete;
   GLBuffer& operator=(const GLBuffer&) = delete;
   GLBuffer(GLBuffer&& other) noexcept;
   GLBuffer& operator=(GLBuffer&& other) noexcept;

   void Bind() const;
   void BindBase(const uint32_t bindingPoint) const;
   void Unbind() const;
   void UploadData(const void* data, const size_t size);
   void UpdateData(const void* data, const size_t size, const size_t offset = 0);
   void* Map(const GLenum access = GL_READ_WRITE);
   void Unmap();

   template<typename T>
   void UploadData(const std::vector<T>& data);
   template<typename T>
   void UpdateData(const std::vector<T>& data, const size_t offset = 0);

   GLuint Get() const;
   Type GetType() const;
   Usage GetUsage() const;
   size_t GetSize() const;
   bool IsValid() const;

private:
   GLuint m_buffer = 0;
   Type m_type;
   Usage m_usage;
   size_t m_size = 0;
};

template<typename T>
void GLBuffer::UploadData(const std::vector<T>& data) {
   UploadData(data.data(), data.size() * sizeof(T));
}

template<typename T>
void GLBuffer::UpdateData(const std::vector<T>& data, const size_t offset) {
   UpdateData(data.data(), data.size() * sizeof(T), offset);
}

