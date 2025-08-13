#include "GLVertexArray.hpp"

#include "GLBuffer.hpp"
#include "../core/Vertex.hpp"

#include <utility>
#include <stdexcept>

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
   :
   m_vao(std::exchange(other.m_vao, 0))
{}

GLVertexArray& GLVertexArray::operator=(GLVertexArray&& other) noexcept {
   if (this != &other) {
      if (m_vao != 0) {
         glDeleteVertexArrays(1, &m_vao);
      }
      m_vao = std::exchange(other.m_vao, 0);
   }
   return *this;
}

void GLVertexArray::Bind() const {
   if (m_vao != 0) {
      glBindVertexArray(m_vao);
   }
}

void GLVertexArray::Unbind() const {
   glBindVertexArray(0);
}

void GLVertexArray::AttachVertexBuffer(const GLBuffer& buffer, const GLuint bindingIndex,
                                       const GLintptr offset, const GLsizei stride) {
   if (m_vao == 0 || !buffer.IsValid()) {
      throw std::runtime_error("Cannot attach buffer to invalid vertex array.");
   }
   if (buffer.GetType() != GLBuffer::Type::Array) {
      throw std::runtime_error("Buffer must be of type Array to attach as vertex buffer.");
   }
   glVertexArrayVertexBuffer(m_vao, bindingIndex, buffer.Get(), offset, stride);
}

void GLVertexArray::AttachElementBuffer(const GLBuffer& buffer) {
   if (m_vao == 0 || !buffer.IsValid()) {
      throw std::runtime_error("Cannot attach buffer to invalid vertex array.");
   }
   if (buffer.GetType() != GLBuffer::Type::Element) {
      throw std::runtime_error("Buffer must be of type Element to attach as element buffer.");
   }
   glVertexArrayElementBuffer(m_vao, buffer.Get());
}

void GLVertexArray::SetupVertexAttributes() {
   if (m_vao == 0) {
      throw std::runtime_error("Cannot setup attributes on invalid vertex array.");
   }
   constexpr GLuint bindingIndex = 0;
   // Position
   EnableAttribute(0);
   SetAttributeFormat(0, 3, GL_FLOAT, GL_FALSE,
                      offsetof(Vertex, position));
   SetAttributeBinding(0, bindingIndex);
   // Normal
   EnableAttribute(1);
   SetAttributeFormat(1, 3, GL_FLOAT, GL_FALSE,
                      offsetof(Vertex, normal));
   SetAttributeBinding(1, bindingIndex);
   // UV
   EnableAttribute(2);
   SetAttributeFormat(2, 2, GL_FLOAT, GL_FALSE,
                      offsetof(Vertex, uv));
   SetAttributeBinding(2, bindingIndex);
}

void GLVertexArray::EnableAttribute(const GLuint index) {
   if (m_vao != 0) {
      glEnableVertexArrayAttrib(m_vao, index);
   }
}

void GLVertexArray::DisableAttribute(const GLuint index) {
   if (m_vao != 0) {
      glDisableVertexArrayAttrib(m_vao, index);
   }
}

void GLVertexArray::SetAttributeFormat(const GLuint index, const GLint size, const GLenum type,
                                       const GLboolean normalized, const GLuint relativeOffset) {
   if (m_vao != 0) {
      glVertexArrayAttribFormat(m_vao, index, size, type, normalized, relativeOffset);
   }
}

void GLVertexArray::SetAttributeBinding(const GLuint index, const GLuint bindingIndex) {
   if (m_vao != 0) {
      glVertexArrayAttribBinding(m_vao, index, bindingIndex);
   }
}

void GLVertexArray::DrawArrays(const GLenum mode, const GLint first, const GLsizei count) const {
   if (m_vao != 0) {
      Bind();
      glDrawArrays(mode, first, count);
   }
}

void GLVertexArray::DrawElements(const GLenum mode, const GLsizei count, const GLenum type, const void* indices) const {
   if (m_vao != 0) {
      Bind();
      glDrawElements(mode, count, type, indices);
   }
}

void GLVertexArray::DrawElementsInstanced(const GLenum mode, const GLsizei count, const GLenum type,
                                          const void* indices, const GLsizei instanceCount) const {
   if (m_vao != 0) {
      Bind();
      glDrawElementsInstanced(mode, count, type, indices, instanceCount);
   }
}

GLuint GLVertexArray::Get() const {
   return m_vao;
}

bool GLVertexArray::IsValid() const {
   return m_vao != 0;
}

