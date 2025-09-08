#include "gl/GLBuffer.hpp"

#include <stdexcept>
#include <utility>

GLBuffer::GLBuffer(const Type type, const Usage usage) : m_type(type), m_usage(usage) {
   glCreateBuffers(1, &m_buffer);
   if (m_buffer == 0) {
      throw std::runtime_error("Failed to create OpenGL buffer");
   }
}

GLBuffer::~GLBuffer() {
   if (m_buffer != 0) {
      glDeleteBuffers(1, &m_buffer);
   }
}

GLBuffer::GLBuffer(GLBuffer&& other) noexcept
    : m_buffer(std::exchange(other.m_buffer, 0)),
      m_type(other.m_type),
      m_usage(other.m_usage),
      m_size(std::exchange(other.m_size, 0)) {}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) noexcept {
   if (this != &other) {
      if (m_buffer != 0) {
         glDeleteBuffers(1, &m_buffer);
      }
      m_buffer = std::exchange(other.m_buffer, 0);
      m_size = std::exchange(other.m_size, 0);
      m_type = other.m_type;
      m_usage = other.m_usage;
   }
   return *this;
}

void GLBuffer::Bind() const noexcept {
   if (m_buffer != 0) {
      glBindBuffer(static_cast<GLenum>(m_type), m_buffer);
   }
}

void GLBuffer::BindBase(const uint32_t bindingPoint) const noexcept {
   if (m_buffer != 0) {
      glBindBufferBase(static_cast<GLenum>(m_type), bindingPoint, m_buffer);
   }
}

void GLBuffer::Unbind(const Type type) noexcept { glBindBuffer(static_cast<GLenum>(type), 0); }

void GLBuffer::UploadData(const void* const data, const size_t size) {
   if (m_buffer == 0) {
      throw std::runtime_error("Cannot upload data to invalid buffer");
   }
   glNamedBufferData(m_buffer, static_cast<GLsizeiptr>(size), data, static_cast<GLenum>(m_usage));
   m_size = size;
}

void GLBuffer::UpdateData(const void* const data, const size_t size, const size_t offset) const {
   if (m_buffer == 0) {
      throw std::runtime_error("Cannot update data in invalid buffer");
   }
   if (offset + size > m_size) {
      throw std::runtime_error("Buffer update would exceed allocated size");
   }
   glNamedBufferSubData(m_buffer, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size),
                        data);
}

void* GLBuffer::Map(const uint32_t access) const {
   if (m_buffer == 0) {
      throw std::runtime_error("Cannot map invalid buffer");
   }
   return glMapNamedBuffer(m_buffer, access);
}

void GLBuffer::Unmap() const noexcept {
   if (m_buffer != 0) {
      glUnmapNamedBuffer(m_buffer);
   }
}
