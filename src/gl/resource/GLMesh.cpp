#include "GLMesh.hpp"

GLMesh::GLMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices)
   :
   m_ebo(GLBuffer::Type::Element, GLBuffer::Usage::StaticDraw),
   m_vbo(GLBuffer::Type::Array, GLBuffer::Usage::StaticDraw),
   m_vao(),
   m_indexCount(indices.size()),
   m_vertexCount(vertices.size())
{
   m_vbo.UploadData(vertices);
   m_ebo.UploadData(indices);
   m_vao.AttachVertexBuffer(m_vbo,0, 0, sizeof(Vertex));
   m_vao.AttachElementBuffer(m_ebo);
   m_vao.SetupVertexAttributes();
}

GLMesh::~GLMesh() = default;

void GLMesh::Draw() const {
   m_vao.DrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT);
}

size_t GLMesh::GetIndexCount() const {
   return m_indexCount;
}

size_t GLMesh::GetVertexCount() const {
   return m_vertexCount;
}
