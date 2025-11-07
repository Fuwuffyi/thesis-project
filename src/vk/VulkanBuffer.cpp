#include "vk/VulkanBuffer.hpp"

#include "vk/VulkanDevice.hpp"

#include <utility>

VulkanBuffer::VulkanBuffer(const VulkanDevice& device, const VkDeviceSize size, const Usage usage,
                           const MemoryType memoryType)
    : m_device(&device), m_allocator(device.GetAllocator()), m_size(size) {
   CreateBuffer(size, usage, memoryType);
}

VulkanBuffer::~VulkanBuffer() {
   if (m_buffer != VK_NULL_HANDLE) {
      if (IsMapped())
         Unmap();
      vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
   }
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : m_device(std::exchange(other.m_device, nullptr)),
      m_allocator(std::exchange(other.m_allocator, VK_NULL_HANDLE)),
      m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE)),
      m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE)),
      m_size(std::exchange(other.m_size, 0)),
      m_mapped(std::exchange(other.m_mapped, nullptr)),
      m_memoryType(std::exchange(other.m_memoryType, MemoryType::CPUToGPU)) {}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
   if (this != &other) {
      if (m_buffer != VK_NULL_HANDLE) {
         if (IsMapped())
            Unmap();
         vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
      }
      m_device = std::exchange(other.m_device, nullptr);
      m_allocator = std::exchange(other.m_allocator, VK_NULL_HANDLE);
      m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
      m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
      m_size = std::exchange(other.m_size, 0);
      m_mapped = std::exchange(other.m_mapped, nullptr);
      m_memoryType = std::exchange(other.m_memoryType, MemoryType::CPUToGPU);
   }
   return *this;
}

void VulkanBuffer::Update(const void* data, const VkDeviceSize size, const VkDeviceSize offset) {
   if (m_memoryType == MemoryType::GPUOnly) {
      abort();
      throw std::runtime_error(
         "Cannot Update GPU-only buffer directly. Use staging buffer + copy.");
   }
   if (!m_mapped)
      throw std::runtime_error("Buffer not mapped for writing");
   std::memcpy(static_cast<char*>(m_mapped) + offset, data, static_cast<size_t>(size));
}

void VulkanBuffer::FlushRange(const VkDeviceSize& offset, const VkDeviceSize& size) const {
   if (!m_mapped)
      return;
   VkMemoryPropertyFlags memFlags = 0;
   vmaGetAllocationMemoryProperties(m_allocator, m_allocation, &memFlags);
   if (!(memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      VmaAllocationInfo allocInfo{};
      vmaGetAllocationInfo(m_allocator, m_allocation, &allocInfo);
      VkMappedMemoryRange range{};
      range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      range.memory = allocInfo.deviceMemory;
      range.offset = allocInfo.offset + offset;
      range.size = size == VK_WHOLE_SIZE ? allocInfo.size : size;
      vkFlushMappedMemoryRanges(m_device->Get(), 1, &range);
   }
}

void* VulkanBuffer::Map() {
   if (m_memoryType == MemoryType::GPUOnly)
      return nullptr;
   if (!m_mapped) {
      const VkResult result = vmaMapMemory(m_allocator, m_allocation, &m_mapped);
      if (result != VK_SUCCESS) {
         m_mapped = nullptr;
         throw std::runtime_error("Failed to map buffer memory: " + std::to_string(result));
      }
   }
   return m_mapped;
}

void VulkanBuffer::Unmap() {
   if (m_mapped) {
      vmaUnmapMemory(m_allocator, m_allocation);
      m_mapped = nullptr;
   }
}

VkBuffer VulkanBuffer::Get() const { return m_buffer; }

VkDeviceSize VulkanBuffer::GetSize() const { return m_size; }

bool VulkanBuffer::IsMapped() const { return m_mapped != nullptr; }

void VulkanBuffer::CreateBuffer(const VkDeviceSize size, const Usage usage,
                                const MemoryType memoryType) {
   VkBufferUsageFlags usageFlags = static_cast<VkBufferUsageFlags>(usage);
   VmaAllocationCreateInfo allocInfo{};
   switch (memoryType) {
      case MemoryType::GPUOnly:
         allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
         break;
      case MemoryType::CPUToGPU:
         allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
         break;
      case MemoryType::GPUToCPU:
         allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
         break;
   }
   VkBufferCreateInfo bufferInfo{};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = size;
   bufferInfo.usage =
      usageFlags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
   bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to create VMA buffer");
   }
}
