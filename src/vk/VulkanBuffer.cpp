#include "VulkanBuffer.hpp"

#include "VulkanDevice.hpp"
#include "VulkanCommandBuffers.hpp"

#include <stdexcept>
#include <cstring>
#include <utility>
#include <algorithm>

VulkanBuffer::VulkanBuffer(const VulkanDevice& device, const VkDeviceSize size, const Usage usage,
                           const MemoryType memoryType)
    : m_device(&device),
      m_size(size),
      m_usage(usage),
      m_memoryProperties(GetMemoryPropertiesFromType(memoryType)) {
   CreateBuffer();
   AllocateMemory();
   if (memoryType == MemoryType::HostVisible || memoryType == MemoryType::HostCoherent) {
      m_mapped = Map();
      m_persistentlyMapped = true;
   }
}

VulkanBuffer::~VulkanBuffer() {
   if (m_mapped) {
      vkUnmapMemory(m_device->Get(), m_memory);
      m_mapped = nullptr;
   }
   if (m_buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(m_device->Get(), m_buffer, nullptr);
      m_buffer = VK_NULL_HANDLE;
   }
   if (m_memory != VK_NULL_HANDLE) {
      vkFreeMemory(m_device->Get(), m_memory, nullptr);
      m_memory = VK_NULL_HANDLE;
   }
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : m_device(std::exchange(other.m_device, nullptr)),
      m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE)),
      m_memory(std::exchange(other.m_memory, VK_NULL_HANDLE)),
      m_size(std::exchange(other.m_size, 0)),
      m_usage(std::exchange(other.m_usage, Usage::Vertex)),
      m_memoryProperties(std::exchange(other.m_memoryProperties, 0)),
      m_mapped(std::exchange(other.m_mapped, nullptr)),
      m_persistentlyMapped(std::exchange(other.m_persistentlyMapped, false)) {}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
   if (this != &other) {
      if (m_mapped) {
         vkUnmapMemory(m_device->Get(), m_memory);
         m_mapped = nullptr;
      }
      if (m_buffer != VK_NULL_HANDLE) {
         vkDestroyBuffer(m_device->Get(), m_buffer, nullptr);
         m_buffer = VK_NULL_HANDLE;
      }
      if (m_memory != VK_NULL_HANDLE) {
         vkFreeMemory(m_device->Get(), m_memory, nullptr);
         m_memory = VK_NULL_HANDLE;
      }
      m_device = std::exchange(other.m_device, nullptr);
      m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
      m_memory = std::exchange(other.m_memory, VK_NULL_HANDLE);
      m_size = std::exchange(other.m_size, 0);
      m_usage = std::exchange(other.m_usage, Usage::Vertex);
      m_memoryProperties = std::exchange(other.m_memoryProperties, 0);
      m_mapped = std::exchange(other.m_mapped, nullptr);
      m_persistentlyMapped = std::exchange(other.m_persistentlyMapped, false);
   }
   return *this;
}

void VulkanBuffer::Update(const void* data, const VkDeviceSize size, const VkDeviceSize offset) {
   if (!data) {
      throw std::invalid_argument("Data cannot be null.");
   }
   if (offset + size > m_size) {
      throw std::out_of_range("Update size exceeds buffer size.");
   }
   // If buffer is persistently mapped, use direct update
   if (m_persistentlyMapped) {
      UpdateMapped(data, size, offset);
      return;
   }
   // For device-local buffers, we need to use staging
   if (!(m_memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
      throw std::runtime_error(
         "Cannot directly update device-local buffer. Use staging buffer or CopyFrom()");
   }
   // Temporarily map, update, and unmap
   void* mapped = Map(size, offset);
   std::memcpy(mapped, data, static_cast<size_t>(size));
   if (!(m_memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      Flush(size, offset);
   }
   Unmap();
}

void VulkanBuffer::UpdateMapped(const void* data, const VkDeviceSize size,
                                const VkDeviceSize offset) {
   if (!m_mapped) {
      throw std::runtime_error("Buffer is not mapped");
   }
   if (offset + size > m_size) {
      throw std::out_of_range("Update size exceeds buffer size");
   }
   std::memcpy(static_cast<char*>(m_mapped) + offset, data, static_cast<size_t>(size));
   if (!(m_memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      Flush(size, offset);
   }
}

void VulkanBuffer::Flush(const VkDeviceSize size, const VkDeviceSize offset) {
   VkMappedMemoryRange range{};
   range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
   range.memory = m_memory;
   range.offset = offset;
   range.size = size;
   vkFlushMappedMemoryRanges(m_device->Get(), 1, &range);
}

void VulkanBuffer::Invalidate(const VkDeviceSize size, const VkDeviceSize offset) {
   VkMappedMemoryRange range{};
   range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
   range.memory = m_memory;
   range.offset = offset;
   range.size = size;
   vkInvalidateMappedMemoryRanges(m_device->Get(), 1, &range);
}

void* VulkanBuffer::Map(const VkDeviceSize size, const VkDeviceSize offset) {
   if (m_mapped) {
      return static_cast<char*>(m_mapped) + offset;
   }
   if (!(m_memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
      throw std::runtime_error("Cannot map non-host-visible memory");
   }
   void* mapped;
   VkResult result = vkMapMemory(m_device->Get(), m_memory, offset, size, 0, &mapped);
   if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to map buffer memory");
   }
   if (!m_persistentlyMapped) {
      return mapped;
   }
   m_mapped = mapped;
   return static_cast<char*>(mapped) + (m_persistentlyMapped ? offset : 0);
}

void VulkanBuffer::Unmap() {
   if (m_persistentlyMapped) {
      return;
   }
   if (m_mapped) {
      vkUnmapMemory(m_device->Get(), m_memory);
      m_mapped = nullptr;
   }
}

void VulkanBuffer::CopyFrom(const VulkanBuffer& srcBuffer, const VkCommandBuffer commandBuffer,
                            VkDeviceSize size, const VkDeviceSize srcOffset,
                            const VkDeviceSize dstOffset) {
   if (size == VK_WHOLE_SIZE) {
      size = std::min(srcBuffer.GetSize() - srcOffset, m_size - dstOffset);
   }
   VkBufferCopy copyRegion{};
   copyRegion.srcOffset = srcOffset;
   copyRegion.dstOffset = dstOffset;
   copyRegion.size = size;
   vkCmdCopyBuffer(commandBuffer, srcBuffer.Get(), m_buffer, 1, &copyRegion);
}

void VulkanBuffer::CopyBuffer(const VulkanDevice& device, const VkCommandPool& commandPool,
                              const VkQueue& queue, const VulkanBuffer& src, VulkanBuffer& dst,
                              const VkDeviceSize size) {
   VulkanCommandBuffers::ExecuteImmediate(
      device, commandPool, queue, [&](const VkCommandBuffer& buf) {
         VkBufferCopy copyRegion{};
         copyRegion.size = size;
         vkCmdCopyBuffer(buf, src.Get(), dst.Get(), 1, &copyRegion);
      });
}

bool VulkanBuffer::SupportsUsage(const Usage usage) const {
   return (static_cast<VkBufferUsageFlags>(m_usage) & static_cast<VkBufferUsageFlags>(usage)) != 0;
}

void VulkanBuffer::CreateBuffer() {
   VkBufferCreateInfo bufferInfo{};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = m_size;
   bufferInfo.usage = static_cast<VkBufferUsageFlags>(m_usage);
   bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   VkResult result = vkCreateBuffer(m_device->Get(), &bufferInfo, nullptr, &m_buffer);
   if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to create buffer");
   }
}

void VulkanBuffer::AllocateMemory() {
   VkMemoryRequirements memRequirements;
   vkGetBufferMemoryRequirements(m_device->Get(), m_buffer, &memRequirements);
   VkMemoryAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize = memRequirements.size;
   allocInfo.memoryTypeIndex = VulkanBuffer::FindMemoryType(
      m_device->GetPhysicalDevice(), memRequirements.memoryTypeBits, m_memoryProperties);
   const VkResult result = vkAllocateMemory(m_device->Get(), &allocInfo, nullptr, &m_memory);
   if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate buffer memory");
   }
   vkBindBufferMemory(m_device->Get(), m_buffer, m_memory, 0);
}

uint32_t VulkanBuffer::FindMemoryType(const VkPhysicalDevice& physicalDevice,
                                      const uint32_t typeFilter,
                                      const VkMemoryPropertyFlags properties) {
   VkPhysicalDeviceMemoryProperties memProperties;
   vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
   for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }
   throw std::runtime_error("Failed to find suitable memory type.");
}

VkMemoryPropertyFlags VulkanBuffer::GetMemoryPropertiesFromType(const MemoryType type) const {
   switch (type) {
      case MemoryType::DeviceLocal:
         return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      case MemoryType::HostVisible:
         return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      case MemoryType::HostCoherent:
         return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      default:
         return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   }
}

VkBuffer VulkanBuffer::Get() const { return m_buffer; }

VkDeviceMemory VulkanBuffer::GetMemory() const { return m_memory; }

VkDeviceSize VulkanBuffer::GetSize() const { return m_size; }

void* VulkanBuffer::GetMapped() const { return m_mapped; }

bool VulkanBuffer::IsMapped() const { return m_mapped != nullptr; }

bool VulkanBuffer::IsPersistentlyMapped() const { return m_persistentlyMapped; }

VulkanBuffer::Usage VulkanBuffer::GetUsage() const { return m_usage; }

VkMemoryPropertyFlags VulkanBuffer::GetMemoryProperties() const { return m_memoryProperties; }
