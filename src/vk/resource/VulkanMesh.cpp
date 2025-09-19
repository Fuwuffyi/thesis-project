#include "VulkanMesh.hpp"

#include "vk/VulkanDevice.hpp"
#include "vk/VulkanCommandBuffers.hpp"

#include <glad/gl.h>
#include <stdexcept>

VulkanMesh::VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                       const VulkanDevice& device)
    : m_indexType(indices.size() <= std::numeric_limits<uint16_t>::max() ? VK_INDEX_TYPE_UINT16
                                                                         : VK_INDEX_TYPE_UINT32),
      m_vertexBuffer(VulkanMesh::CreateVertexBuffer(vertices, device)),
      m_indexBuffer(VulkanMesh::CreateIndexBuffer(indices, device)),
      m_indexCount(indices.size()),
      m_vertexCount(vertices.size()) {}

VulkanMesh::~VulkanMesh() = default;

ResourceType VulkanMesh::GetType() const noexcept { return ResourceType::Mesh; }

size_t VulkanMesh::GetMemoryUsage() const noexcept {
   return (m_vertexCount * sizeof(Vertex)) +
          (m_indexCount *
           (m_indexType == VK_INDEX_TYPE_UINT32 ? sizeof(uint32_t) : sizeof(uint16_t)));
}

bool VulkanMesh::IsValid() const noexcept {
   // NOTE: Handled by RAII
   return true;
}

void* VulkanMesh::GetNativeHandle() const { return reinterpret_cast<void*>(m_vertexBuffer.Get()); }

VulkanBuffer VulkanMesh::CreateVertexBuffer(const std::vector<Vertex>& vertices,
                                            const VulkanDevice& device) {
   VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
   VulkanBuffer staging(device, bufferSize, VulkanBuffer::Usage::Vertex,
                        VulkanBuffer::MemoryType::CPUToGPU);
   staging.Map();
   staging.Update(vertices.data(), bufferSize);
   VulkanBuffer vertexBuffer(device, bufferSize, VulkanBuffer::Usage::Vertex,
                             VulkanBuffer::MemoryType::GPUOnly);
   VulkanCommandBuffers::ExecuteImmediate(
      device, device.GetCommandPool(), device.GetGraphicsQueue(),
      [&vertexBuffer, &staging, bufferSize](const VkCommandBuffer& cmd) {
         VkBufferCopy copyRegion{};
         copyRegion.srcOffset = 0;
         copyRegion.dstOffset = 0;
         copyRegion.size = bufferSize;
         vkCmdCopyBuffer(cmd, staging.Get(), vertexBuffer.Get(), 1, &copyRegion);
      });
   return vertexBuffer;
}

VulkanBuffer VulkanMesh::CreateIndexBuffer(const std::vector<uint32_t>& indices,
                                           const VulkanDevice& device) {
   VkDeviceSize bufferSize;
   std::vector<uint8_t> indexData;
   if (indices.size() <= std::numeric_limits<uint16_t>::max()) {
      std::vector<uint16_t> indices16(indices.begin(), indices.end());
      bufferSize = sizeof(uint16_t) * indices.size();
      indexData.resize(bufferSize);
      std::memcpy(indexData.data(), indices16.data(), bufferSize);
   } else {
      bufferSize = sizeof(uint32_t) * indices.size();
      indexData.resize(bufferSize);
      std::memcpy(indexData.data(), indices.data(), bufferSize);
   }
   VulkanBuffer staging(device, bufferSize, VulkanBuffer::Usage::Index,
                        VulkanBuffer::MemoryType::CPUToGPU);
   staging.Map();
   staging.Update(indexData.data(), bufferSize);
   VulkanBuffer indexBuffer(device, bufferSize, VulkanBuffer::Usage::Index,
                            VulkanBuffer::MemoryType::GPUOnly);
   VulkanCommandBuffers::ExecuteImmediate(
      device, device.GetCommandPool(), device.GetGraphicsQueue(), [&](const VkCommandBuffer& cmd) {
         VkBufferCopy copyRegion{0, 0, bufferSize};
         vkCmdCopyBuffer(cmd, staging.Get(), indexBuffer.Get(), 1, &copyRegion);
      });
   return indexBuffer;
}

void VulkanMesh::Draw() const {
   throw std::runtime_error("Method not implemented for VulkanMesh. (Draw())");
}

void VulkanMesh::Draw(const VkCommandBuffer& cmd) const {
   const VkBuffer vertexBuffers[] = {m_vertexBuffer.Get()};
   const VkDeviceSize offsets[] = {0};
   vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
   vkCmdBindIndexBuffer(cmd, m_indexBuffer.Get(), 0, m_indexType);
   vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_indexCount), 1, 0, 0, 0);
}

size_t VulkanMesh::GetIndexCount() const { return m_indexCount; }

size_t VulkanMesh::GetVertexCount() const { return m_vertexCount; }
