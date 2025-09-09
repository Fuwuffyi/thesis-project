#include "gl/resource/GLMesh.hpp"

#include "core/Vertex.hpp"

GLMesh::GLMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) noexcept
    : m_ebo(GLBuffer::Type::Element, GLBuffer::Usage::StaticDraw),
      m_vbo(GLBuffer::Type::Array, GLBuffer::Usage::StaticDraw),
      m_vao(),
      m_indexCount(indices.size()),
      m_vertexCount(vertices.size()) {
   if (!vertices.empty())
      m_vbo.UploadData(std::span(vertices.data(), vertices.size()));
   if (!indices.empty())
      m_ebo.UploadData(std::span(indices.data(), indices.size()));
   m_vao.AttachVertexBuffer(m_vbo, 0, 0, sizeof(Vertex));
   m_vao.AttachElementBuffer(m_ebo);
   m_vao.SetupVertexAttributes();
}

size_t GLMesh::GetMemoryUsage() const noexcept {
   return (m_vertexCount * sizeof(Vertex)) + (m_indexCount * sizeof(uint32_t));
}

bool GLMesh::IsValid() const noexcept {
   return m_vao.IsValid() && m_vbo.IsValid() && m_ebo.IsValid();
}

void GLMesh::Draw() const noexcept {
   m_vao.DrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT);
}

void GLMesh::Draw(const uint32_t drawType) const {
   m_vao.DrawElements(drawType, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT);
}

void* GLMesh::GetNativeHandle() const noexcept {
   return reinterpret_cast<void*>(static_cast<uintptr_t>(m_vao.Get()));
}
