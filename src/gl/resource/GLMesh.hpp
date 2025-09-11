#pragma once

#include "core/resource/IMesh.hpp"

#include "gl/GLBuffer.hpp"
#include "gl/GLVertexArray.hpp"

#include <vector>

struct Vertex;

class GLMesh final : public IMesh {
  public:
   GLMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) noexcept;
   ~GLMesh() override = default;

   GLMesh(const GLMesh&) = delete;
   GLMesh& operator=(const GLMesh&) = delete;

   [[nodiscard]] constexpr ResourceType GetType() const noexcept override {
      return ResourceType::Mesh;
   }
   [[nodiscard]] size_t GetMemoryUsage() const noexcept override;
   [[nodiscard]] bool IsValid() const noexcept override;

   void Draw() const noexcept override;
   void Draw(const uint32_t drawType) const;
   [[nodiscard]] constexpr size_t GetVertexCount() const noexcept override { return m_vertexCount; }
   [[nodiscard]] constexpr size_t GetIndexCount() const noexcept override { return m_indexCount; }
   [[nodiscard]] void* GetNativeHandle() const noexcept override;

  private:
   GLBuffer m_ebo;
   GLBuffer m_vbo;
   GLVertexArray m_vao;
   uint32_t m_indexType{GL_UNSIGNED_INT};

   size_t m_indexCount{0};
   size_t m_vertexCount{0};
};
