#include "gl/GLVertexArray.hpp"

#include "core/Vertex.hpp"

#include "gl/GLBuffer.hpp"

#include <glad/gl.h>
#include <stdexcept>
#include <utility>

GLVertexArray::GLVertexArray() {
   glCreateVertexArrays(1, &m_vao);
   if (m_vao == 0) {
      throw std::runtime_error("Failed to create OpenGL vertex array");
   }
}

GLVertexArray::~GLVertexArray() {
   if (m_vao != 0) {
      glDeleteVertexArrays(1, &m_vao);
   }
}

GLVertexArray::GLVertexArray(GLVertexArray&& other) noexcept
    : m_vao(std::exchange(other.m_vao, 0)) {}

GLVertexArray& GLVertexArray::operator=(GLVertexArray&& other) noexcept {
   if (this != &other) {
      if (m_vao != 0) {
         glDeleteVertexArrays(1, &m_vao);
      }
      m_vao = std::exchange(other.m_vao, 0);
   }
   return *this;
}

void GLVertexArray::Bind() const noexcept {
   if (m_vao != 0) {
      glBindVertexArray(m_vao);
   }
}

void GLVertexArray::Unbind() noexcept { glBindVertexArray(0); }

void GLVertexArray::AttachVertexBuffer(const GLBuffer& buffer, const uint32_t bindingIndex,
                                       const size_t offset, const size_t stride) const {
   if (m_vao == 0 || !buffer.IsValid()) {
      throw std::runtime_error("Cannot attach buffer to invalid vertex array");
   }
   if (buffer.GetType() != GLBuffer::Type::Array) {
      throw std::runtime_error("Buffer must be of type Array to attach as vertex buffer");
   }
   glVertexArrayVertexBuffer(m_vao, bindingIndex, buffer.Get(), offset, stride);
}

void GLVertexArray::AttachElementBuffer(const GLBuffer& buffer) const {
   if (m_vao == 0 || !buffer.IsValid()) {
      throw std::runtime_error("Cannot attach buffer to invalid vertex array");
   }
   if (buffer.GetType() != GLBuffer::Type::Element) {
      throw std::runtime_error("Buffer must be of type Element to attach as element buffer");
   }
   glVertexArrayElementBuffer(m_vao, buffer.Get());
}

void GLVertexArray::SetupVertexAttributes() const {
   if (m_vao == 0) {
      throw std::runtime_error("Cannot setup attributes on invalid vertex array");
   }
   constexpr uint32_t bindingIndex = 0;
   // Position attribute (location 0)
   EnableAttribute(0);
   SetAttributeFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
   SetAttributeBinding(0, bindingIndex);
   // Normal attribute (location 1)
   EnableAttribute(1);
   SetAttributeFormat(1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
   SetAttributeBinding(1, bindingIndex);
   // UV attribute (location 2)
   EnableAttribute(2);
   SetAttributeFormat(2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
   SetAttributeBinding(2, bindingIndex);
}

void GLVertexArray::EnableAttribute(const uint32_t index) const noexcept {
   if (m_vao != 0) {
      glEnableVertexArrayAttrib(m_vao, index);
   }
}

void GLVertexArray::DisableAttribute(const uint32_t index) const noexcept {
   if (m_vao != 0) {
      glDisableVertexArrayAttrib(m_vao, index);
   }
}

void GLVertexArray::SetAttributeFormat(const uint32_t index, const size_t size,
                                       const uint32_t type, const bool normalized,
                                       const uint32_t relativeOffset) const noexcept {
   if (m_vao != 0) {
      glVertexArrayAttribFormat(m_vao, index, size, type, normalized, relativeOffset);
   }
}

void GLVertexArray::SetAttributeBinding(const uint32_t index,
                                        const uint32_t bindingIndex) const noexcept {
   if (m_vao != 0) {
      glVertexArrayAttribBinding(m_vao, index, bindingIndex);
   }
}

void GLVertexArray::DrawArrays(const uint32_t mode, const uint32_t first,
                               const size_t count) const noexcept {
   if (m_vao != 0) {
      Bind();
      glDrawArrays(mode, first, count);
   }
}

void GLVertexArray::DrawElements(const uint32_t mode, const size_t count, const uint32_t type,
                                 const void* indices) const noexcept {
   if (m_vao != 0) {
      Bind();
      glDrawElements(mode, count, type, indices);
   }
}

void GLVertexArray::DrawElementsInstanced(const uint32_t mode, const size_t count,
                                          const uint32_t type, const void* indices,
                                          const size_t instanceCount) const noexcept {
   if (m_vao != 0) {
      Bind();
      glDrawElementsInstanced(mode, count, type, indices, instanceCount);
   }
}
