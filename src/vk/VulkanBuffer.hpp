#pragma once

#include <vulkan/vulkan.h>

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

   enum class MemoryType {
      DeviceLocal,
      HostVisible,
      HostCoherent
   };

   VulkanBuffer(const VulkanDevice& device, const VkDeviceSize size, const Usage usage, const MemoryType memoryType);
   ~VulkanBuffer();

   VulkanBuffer(const VulkanBuffer&) = delete;
   VulkanBuffer& operator=(const VulkanBuffer&) = delete;
   VulkanBuffer(VulkanBuffer&& other) noexcept;
   VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

   void Update(const void* data, const VkDeviceSize size, const VkDeviceSize offset = 0);
   void UpdateMapped(const void* data, const VkDeviceSize size, const VkDeviceSize offset = 0);
   void Flush(const VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize offset = 0);
   void Invalidate(const VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize offset = 0);

   void* Map(const VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize offset = 0);
   void Unmap();

   void CopyFrom(const VulkanBuffer& srcBuffer, const VkCommandBuffer commandBuffer,
                 VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize srcOffset = 0, const VkDeviceSize dstOffset = 0);

   static void CopyBuffer(const VulkanDevice& device, const VkCommandPool& commandPool, const VkQueue& queue,
                          const VulkanBuffer& src, VulkanBuffer& dst,
                          const VkDeviceSize size = VK_WHOLE_SIZE);

   VkBuffer Get() const;
   VkDeviceMemory GetMemory() const;
   VkDeviceSize GetSize() const;
   void* GetMapped() const;
   bool IsMapped() const;
   bool IsPersistentlyMapped() const;
   Usage GetUsage() const;
   VkMemoryPropertyFlags GetMemoryProperties() const;
   bool SupportsUsage(Usage usage) const;
   // TODO: Maybe move in util?
   static uint32_t FindMemoryType(const VkPhysicalDevice& physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties);
private:
   void CreateBuffer();
   void AllocateMemory();
   VkMemoryPropertyFlags GetMemoryPropertiesFromType(const MemoryType type) const;
private:
   const VulkanDevice* m_device = nullptr;
   VkBuffer m_buffer = VK_NULL_HANDLE;
   VkDeviceMemory m_memory = VK_NULL_HANDLE;
   VkDeviceSize m_size = 0;
   Usage m_usage;
   VkMemoryPropertyFlags m_memoryProperties;
   void* m_mapped = nullptr;
   bool m_persistentlyMapped = false;
};

inline VulkanBuffer::Usage operator|(VulkanBuffer::Usage a, VulkanBuffer::Usage b) {
   return static_cast<VulkanBuffer::Usage>(
      static_cast<VkBufferUsageFlags>(a) | static_cast<VkBufferUsageFlags>(b)
   );
}

inline VulkanBuffer::Usage operator&(VulkanBuffer::Usage a, VulkanBuffer::Usage b) {
   return static_cast<VulkanBuffer::Usage>(
      static_cast<VkBufferUsageFlags>(a) & static_cast<VkBufferUsageFlags>(b)
   );
}

