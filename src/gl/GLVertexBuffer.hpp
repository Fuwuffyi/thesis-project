#pragma once

#include <cstdint>
#include <vector>

struct Vertex;

class GLVertexBuffer {
public:
   GLVertexBuffer();
   ~GLVertexBuffer();

   GLVertexBuffer(const GLVertexBuffer&) = delete;
   GLVertexBuffer& operator=(const GLVertexBuffer&) = delete;
   GLVertexBuffer(GLVertexBuffer&& other) noexcept;
   GLVertexBuffer& operator=(GLVertexBuffer&& other) noexcept;

   void UploadData(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
   void Draw() const;
private:
   uint32_t GenerateVertexBuffer();
   uint32_t GenerateVertexArray();
   uint32_t GenerateElementBuffer();
   void SetupVertexAttributes();
private:
   std::vector<Vertex> m_vertices;
   std::vector<uint16_t> m_indices;
   uint32_t m_vertexBuffer;
   uint32_t m_vertexArray;
   uint32_t m_elementBuffer;
};
