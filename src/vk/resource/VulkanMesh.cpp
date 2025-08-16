#include "VulkanMesh.hpp"
#include <glad/gl.h>

VulkanMesh::VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices,
                       const VulkanDevice& device, const VkCommandPool& commandPool, const VkQueue& queue)
   :
   m_vertexBuffer(VulkanMesh::CreateVertexBuffer(vertices, device, commandPool, queue)),
   m_indexBuffer(VulkanMesh::CreateIndexBuffer(indices, device, commandPool, queue)),
   m_indexCount(indices.size()),
   m_vertexCount(vertices.size())
{}

VulkanMesh::~VulkanMesh() = default;

void VulkanMesh::Draw(const VkCommandBuffer& cmd) const {
   VkBuffer vertexBuffers[] = { m_vertexBuffer.Get() };
   VkDeviceSize offsets[] = { 0 };
   vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
   vkCmdBindIndexBuffer(cmd, m_indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT16);
   vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_indexCount), 1, 0, 0, 0);
}

VulkanBuffer VulkanMesh::CreateVertexBuffer(const std::vector<Vertex>& vertices, const VulkanDevice& device,
                                            const VkCommandPool& commandPool, const VkQueue& queue) {
   const VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
   VulkanBuffer stagingBuffer(
      device, bufferSize, VulkanBuffer::Usage::TransferSrc,
      VulkanBuffer::MemoryType::HostVisible
   );
   stagingBuffer.Update(vertices.data(), bufferSize);
   VulkanBuffer vertexBuffer(
      device, bufferSize,
      VulkanBuffer::Usage::TransferDst | VulkanBuffer::Usage::Vertex,
      VulkanBuffer::MemoryType::DeviceLocal
   );
   VulkanBuffer::CopyBuffer(device, commandPool, queue, stagingBuffer, vertexBuffer, bufferSize);
   return vertexBuffer;
}

VulkanBuffer VulkanMesh::CreateIndexBuffer(const std::vector<uint16_t>& indices, const VulkanDevice& device,
                                           const VkCommandPool& commandPool, const VkQueue& queue) {
   const VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();
   VulkanBuffer stagingBuffer(
      device, bufferSize, VulkanBuffer::Usage::TransferSrc,
      VulkanBuffer::MemoryType::HostVisible
   );
   stagingBuffer.Update(indices.data(), bufferSize);
   VulkanBuffer indexBuffer(
      device, bufferSize,
      VulkanBuffer::Usage::TransferDst | VulkanBuffer::Usage::Index,
      VulkanBuffer::MemoryType::DeviceLocal
   );
   VulkanBuffer::CopyBuffer(device, commandPool, queue, stagingBuffer, indexBuffer, bufferSize);
   return indexBuffer;
}

size_t VulkanMesh::GetIndexCount() const {
   return m_indexCount;
}

size_t VulkanMesh::GetVertexCount() const {
   return m_vertexCount;
}

