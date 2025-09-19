#pragma once

#include "core/Vertex.hpp"
#include "core/resource/IMesh.hpp"

#include "vk/VulkanBuffer.hpp"

#include <vector>
#include <cstddef>

class VulkanMesh : public IMesh {
  public:
   VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
              const VulkanDevice& device);
   ~VulkanMesh();

   VulkanMesh(const VulkanMesh&) = delete;
   VulkanMesh& operator=(const VulkanMesh&) = delete;
   VulkanMesh(VulkanMesh&& other) noexcept = default;
   VulkanMesh& operator=(VulkanMesh&& other) noexcept = default;

   ResourceType GetType() const noexcept override;
   size_t GetMemoryUsage() const noexcept override;
   bool IsValid() const noexcept override;

   void Draw() const;
   size_t GetIndexCount() const;
   size_t GetVertexCount() const;
   void* GetNativeHandle() const override;

   void Draw(const VkCommandBuffer& commandBuffer) const;

  private:
   VulkanBuffer CreateVertexBuffer(const std::vector<Vertex>& vertices, const VulkanDevice& device);
   VulkanBuffer CreateIndexBuffer(const std::vector<uint32_t>& indices, const VulkanDevice& device);

  private:
   VulkanBuffer m_vertexBuffer;
   VulkanBuffer m_indexBuffer;
   VkIndexType m_indexType{VK_INDEX_TYPE_UINT32};

   size_t m_indexCount;
   size_t m_vertexCount;
};
