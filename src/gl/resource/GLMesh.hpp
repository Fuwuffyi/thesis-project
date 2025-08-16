#pragma once

#include "../../core/Vertex.hpp"
#include "../GLBuffer.hpp"
#include "../GLVertexArray.hpp"

#include <cstddef>

class GLMesh {
public:
   GLMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
   ~GLMesh();

   void Draw() const;

   size_t GetIndexCount() const;
   size_t GetVertexCount() const;

private:
   GLBuffer m_ebo;
   GLBuffer m_vbo;
   GLVertexArray m_vao;

   size_t m_indexCount;
   size_t m_vertexCount;
};

