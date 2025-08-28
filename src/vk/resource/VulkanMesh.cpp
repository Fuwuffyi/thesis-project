#include "VulkanMesh.hpp"

#include "../VulkanDevice.hpp"

#include <glad/gl.h>
#include <stdexcept>

VulkanMesh::VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                       const VulkanDevice& device)
   :
   m_vertexBuffer(VulkanMesh::CreateVertexBuffer(vertices, device)),
   m_indexBuffer(VulkanMesh::CreateIndexBuffer(indices, device)),
   m_indexCount(indices.size()),
   m_vertexCount(vertices.size())
{}

VulkanMesh::~VulkanMesh() = default;

ResourceType VulkanMesh::GetType() const {
   return ResourceType::Mesh;
}

size_t VulkanMesh::GetMemoryUsage() const {
   return (m_vertexCount * sizeof(Vertex)) + (m_indexCount * sizeof(uint32_t));
}

bool VulkanMesh::IsValid() const {
   // NOTE: Handled by RAII
   return true;
}

void* VulkanMesh::GetNativeHandle() const {
   return reinterpret_cast<void*>(m_vertexBuffer.Get());
}

VulkanBuffer VulkanMesh::CreateVertexBuffer(const std::vector<Vertex>& vertices, const VulkanDevice& device) {
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
   VulkanBuffer::CopyBuffer(device, device.GetCommandPool(), device.GetGraphicsQueue(),
                            stagingBuffer, vertexBuffer, bufferSize);
   return vertexBuffer;
}

VulkanBuffer VulkanMesh::CreateIndexBuffer(const std::vector<uint32_t>& indices, const VulkanDevice& device) {
   const VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
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
   VulkanBuffer::CopyBuffer(device, device.GetCommandPool(), device.GetGraphicsQueue(),
                            stagingBuffer, indexBuffer, bufferSize);
   return indexBuffer;
}

void VulkanMesh::Draw() const {
   throw std::runtime_error("Method not implemented for VulkanMesh. (Draw())");
}

void VulkanMesh::Draw(const VkCommandBuffer& cmd) const {
   const VkBuffer vertexBuffers[] = { m_vertexBuffer.Get() };
   const VkDeviceSize offsets[] = { 0 };
   vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
   vkCmdBindIndexBuffer(cmd, m_indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT32);
   vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_indexCount), 1, 0, 0, 0);
}

size_t VulkanMesh::GetIndexCount() const {
   return m_indexCount;
}

size_t VulkanMesh::GetVertexCount() const {
   return m_vertexCount;
}

