#pragma once

#include "../../core/Vertex.hpp"
#include "core/resource/IMesh.hpp"

#include "vk/VulkanBuffer.hpp"

#include <vector>
#include <cstddef>

class VulkanMesh : public IMesh {
public:
   VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices,
              const VulkanDevice& device);
   ~VulkanMesh();

   VulkanMesh(const VulkanMesh&) = delete;
   VulkanMesh& operator=(const VulkanMesh&) = delete;

   ResourceType GetType() const override;
   size_t GetMemoryUsage() const override;
   bool IsValid() const override;


   void Draw() const;
   size_t GetIndexCount() const;
   size_t GetVertexCount() const;
   void* GetNativeHandle() const override;

   void Draw(const VkCommandBuffer& commandBuffer) const;
   const VulkanBuffer& GetVertexBuffer() const;
   const VulkanBuffer& GetIndexBuffer() const;
private:
   static VulkanBuffer CreateVertexBuffer(const std::vector<Vertex>& vertices, const VulkanDevice& device);
   static VulkanBuffer CreateIndexBuffer(const std::vector<uint16_t>& indices, const VulkanDevice& device);

private:
   VulkanBuffer m_vertexBuffer;
   VulkanBuffer m_indexBuffer;

   size_t m_indexCount;
   size_t m_vertexCount;
};

