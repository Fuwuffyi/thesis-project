#pragma once

#include "../../core/resource/IMesh.hpp"

#include "../../core/Vertex.hpp"
#include "../GLBuffer.hpp"
#include "../GLVertexArray.hpp"

#include <cstddef>

class GLMesh : public IMesh {
  public:
   GLMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
   ~GLMesh();

   GLMesh(const GLMesh&) = delete;
   GLMesh& operator=(const GLMesh&) = delete;

   ResourceType GetType() const override;
   size_t GetMemoryUsage() const override;
   bool IsValid() const override;

   void Draw() const override;
   size_t GetVertexCount() const override;
   size_t GetIndexCount() const override;
   void* GetNativeHandle() const override;

  private:
   GLBuffer m_ebo;
   GLBuffer m_vbo;
   GLVertexArray m_vao;

   size_t m_indexCount;
   size_t m_vertexCount;
};
