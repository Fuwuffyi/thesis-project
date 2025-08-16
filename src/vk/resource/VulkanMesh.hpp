#pragma once

#include "../../core/Vertex.hpp"
#include "vk/VulkanBuffer.hpp"

#include <vector>
#include <cstddef>

class VulkanMesh {
public:
   VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices,
              const VulkanDevice& device, const VkCommandPool& commandPool, const VkQueue& queue);
   ~VulkanMesh();

   void Draw(const VkCommandBuffer& cmd) const;

   size_t GetIndexCount() const;
   size_t GetVertexCount() const;
private:
   static VulkanBuffer CreateVertexBuffer(const std::vector<Vertex>& vertices, const VulkanDevice& device,
                                          const VkCommandPool& commandPool, const VkQueue& queue);
   static VulkanBuffer CreateIndexBuffer(const std::vector<uint16_t>& indices, const VulkanDevice& device,
                                         const VkCommandPool& commandPool, const VkQueue& queue);

private:
   VulkanBuffer m_vertexBuffer;
   VulkanBuffer m_indexBuffer;

   size_t m_indexCount;
   size_t m_vertexCount;
};

