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
   if (!indices.empty()) {
      if (indices.size() <= std::numeric_limits<uint16_t>::max()) {
         m_indexType = GL_UNSIGNED_SHORT;
         const std::vector<uint16_t> indices16(indices.begin(), indices.end());
         m_ebo.UploadData(indices16.data(), indices16.size() * sizeof(uint16_t));
      } else {
         m_indexType = GL_UNSIGNED_INT;
         m_ebo.UploadData(indices.data(), indices.size() * sizeof(uint32_t));
      }
   }
   m_vao.AttachVertexBuffer(m_vbo, 0, 0, sizeof(Vertex));
   m_vao.AttachElementBuffer(m_ebo);
   m_vao.SetupVertexAttributes();
}

size_t GLMesh::GetMemoryUsage() const noexcept {
   return (m_vertexCount * sizeof(Vertex)) +
          (m_indexCount * (m_indexType == GL_UNSIGNED_INT ? sizeof(uint32_t) : sizeof(uint16_t)));
}

bool GLMesh::IsValid() const noexcept {
   return m_vao.IsValid() && m_vbo.IsValid() && m_ebo.IsValid();
}

void GLMesh::Draw() const noexcept { Draw(GL_TRIANGLES); }

void GLMesh::Draw(const uint32_t drawType) const {
   m_vao.DrawElements(drawType, m_indexCount, m_indexType);
}

void* GLMesh::GetNativeHandle() const noexcept {
   return reinterpret_cast<void*>(static_cast<uintptr_t>(m_vao.Get()));
}
