#include "GLVertexBuffer.hpp"

#include "../core/Vertex.hpp"

#include <cstring>
#include <glad/gl.h>
#include <utility>

GLVertexBuffer::GLVertexBuffer()
   :
   m_vertices(),
   m_indices(),
   m_vertexBuffer(GenerateVertexBuffer()),
   m_vertexArray(GenerateVertexArray()),
   m_elementBuffer(GenerateElementBuffer())
{}

GLVertexBuffer::~GLVertexBuffer() {
   if (m_elementBuffer) glDeleteBuffers(1, &m_elementBuffer);
   if (m_vertexBuffer) glDeleteBuffers(1, &m_vertexBuffer);
   if (m_vertexArray) glDeleteVertexArrays(1, &m_vertexArray);
}

GLVertexBuffer::GLVertexBuffer(GLVertexBuffer&& other) noexcept
   :
   m_vertexBuffer(std::exchange(other.m_vertexBuffer, 0)),
   m_vertexArray(std::exchange(other.m_vertexArray, 0)),
   m_elementBuffer(std::exchange(other.m_elementBuffer, 0))
{}

GLVertexBuffer&  GLVertexBuffer::operator=(GLVertexBuffer&& other) noexcept {
   if (this != &other) {
      m_vertexBuffer = std::exchange(other.m_vertexBuffer, 0);
      m_vertexArray = std::exchange(other.m_vertexArray, 0);
      m_elementBuffer = std::exchange(other.m_elementBuffer, 0);
   }
   return *this;
}

void GLVertexBuffer::UploadData(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) {
   glNamedBufferData(m_vertexBuffer,
                     static_cast<int64_t>(vertices.size() * sizeof(Vertex)),
                     vertices.data(),
                     GL_STATIC_DRAW);
   glNamedBufferData(m_elementBuffer,
                     static_cast<int64_t>(indices.size() * sizeof(uint16_t)),
                     indices.data(),
                     GL_STATIC_DRAW);
   SetupVertexAttributes();
   m_vertices = vertices;
   m_indices = indices;
}

void GLVertexBuffer::Draw() const {
   glBindVertexArray(m_vertexArray);
   glDrawElements(GL_TRIANGLES, static_cast<int32_t>(m_indices.size()), GL_UNSIGNED_SHORT, nullptr);
}

uint32_t GLVertexBuffer::GenerateVertexBuffer() {
   uint32_t id = 0;
   glCreateBuffers(1, &id);
   return id;
}

uint32_t GLVertexBuffer::GenerateVertexArray() {
   uint32_t id = 0;
   glCreateVertexArrays(1, &id);
   return id;
}

uint32_t GLVertexBuffer::GenerateElementBuffer() {
   uint32_t id = 0;
   glCreateBuffers(1, &id);
   glVertexArrayElementBuffer(m_vertexArray, id);
   return id;
}

void GLVertexBuffer::SetupVertexAttributes() {
   constexpr GLuint bindingIndex = 0;
   glVertexArrayVertexBuffer(m_vertexArray, bindingIndex, m_vertexBuffer, 0, sizeof(Vertex));
   // Position (location = 0)
   glEnableVertexArrayAttrib(m_vertexArray, 0);
   glVertexArrayAttribFormat(m_vertexArray, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
   glVertexArrayAttribBinding(m_vertexArray, 0, bindingIndex);
   // Normal (location = 1)
   glEnableVertexArrayAttrib(m_vertexArray, 1);
   glVertexArrayAttribFormat(m_vertexArray, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
   glVertexArrayAttribBinding(m_vertexArray, 1, bindingIndex);
   // UV (location = 2)
   glEnableVertexArrayAttrib(m_vertexArray, 2);
   glVertexArrayAttribFormat(m_vertexArray, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
   glVertexArrayAttribBinding(m_vertexArray, 2, bindingIndex);
}
