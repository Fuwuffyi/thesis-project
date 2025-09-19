#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class VulkanDevice;

class VulkanBuffer {
  public:
   enum class Usage : VkBufferUsageFlags {
      Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      TransferSrc = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      TransferDst = VK_BUFFER_USAGE_TRANSFER_DST_BIT
   };

   enum class MemoryType { GPUOnly, CPUToGPU, GPUToCPU };

   VulkanBuffer(const VulkanDevice& device, const VkDeviceSize size, const Usage usage,
                const MemoryType memoryType);
   ~VulkanBuffer();

   VulkanBuffer(const VulkanBuffer&) = delete;
   VulkanBuffer& operator=(const VulkanBuffer&) = delete;
   VulkanBuffer(VulkanBuffer&& other) noexcept;
   VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

   void Update(const void* data, const VkDeviceSize size, const VkDeviceSize offset = 0);
   void* Map();
   void Unmap();

   VkBuffer Get() const;
   VkDeviceSize GetSize() const;
   bool IsMapped() const;

  private:
   void CreateBuffer(const VkDeviceSize size, const Usage usage, const MemoryType memoryType);

  private:
   const VulkanDevice* m_device{nullptr};
   VmaAllocator m_allocator{VK_NULL_HANDLE};
   VkBuffer m_buffer{VK_NULL_HANDLE};
   VmaAllocation m_allocation{VK_NULL_HANDLE};
   VkDeviceSize m_size{0};
   MemoryType m_memoryType{MemoryType::CPUToGPU};
   void* m_mapped{nullptr};
};
