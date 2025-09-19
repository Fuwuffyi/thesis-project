#include "vk/VulkanDevice.hpp"
#include "vk/VulkanInstance.hpp"
#include "vk/VulkanSurface.hpp"

#include <stdexcept>

VulkanDevice::VulkanDevice(const VulkanInstance& instance, const VulkanSurface& surface) {
   VkPhysicalDeviceFeatures required_features{};
   required_features.samplerAnisotropy = VK_TRUE;
   VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
   bufferDeviceAddressFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
   bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
   // Physical device
   vkb::PhysicalDeviceSelector selector{instance.GetVkbInstance()};
   auto phys_ret = selector.set_surface(surface.Get())
                      .set_minimum_version(1, 1)
                      .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                      .allow_any_gpu_device_type(false)
                      .require_present()
                      .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
                      .set_required_features(required_features)
                      .select();
   if (!phys_ret)
      throw std::runtime_error("Failed to select physical device: " + phys_ret.error().message());
   auto vkb_physical_device = phys_ret.value();
   m_physicalDevice = vkb_physical_device.physical_device;
   // Logical device
   vkb::DeviceBuilder device_builder{vkb_physical_device};
   device_builder.add_pNext(&bufferDeviceAddressFeatures);
   auto dev_ret = device_builder.build();
   if (!dev_ret)
      throw std::runtime_error("Failed to create logical device: " + dev_ret.error().message());
   m_vkbDevice = dev_ret.value();
   m_device = m_vkbDevice.device;
   m_ownsDevice = true;
   // Create the vulkan allocator
   CreateAllocator(instance, bufferDeviceAddressFeatures);
   // Queues
   const auto graphics_queue_ret = m_vkbDevice.get_queue(vkb::QueueType::graphics);
   const auto present_queue_ret = m_vkbDevice.get_queue(vkb::QueueType::present);
   if (!graphics_queue_ret) {
      throw std::runtime_error("Failed to get graphics queue: " +
                               graphics_queue_ret.error().message());
   }
   if (!present_queue_ret) {
      throw std::runtime_error("Failed to get present queue: " +
                               present_queue_ret.error().message());
   }
   m_graphicsQueue = graphics_queue_ret.value();
   m_presentQueue = present_queue_ret.value();
   // Get queue family indices
   const auto graphics_family_ret = m_vkbDevice.get_queue_index(vkb::QueueType::graphics);
   const auto present_family_ret = m_vkbDevice.get_queue_index(vkb::QueueType::present);
   if (graphics_family_ret && present_family_ret) {
      m_queueFamilies.graphicsFamily = graphics_family_ret.value();
      m_queueFamilies.presentFamily = present_family_ret.value();
   }
   CreateCommandPool();
}

VulkanDevice::~VulkanDevice() {
   if (m_commandPool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(m_device, m_commandPool, nullptr);
   }
   if (m_allocator != VK_NULL_HANDLE) {
      vmaDestroyAllocator(m_allocator);
   }
   if (m_ownsDevice) {
      vkb::destroy_device(m_vkbDevice);
   }
}

VulkanDevice::VulkanDevice(VulkanDevice&& other) noexcept
    : m_vkbDevice(std::move(other.m_vkbDevice)),
      m_device(other.m_device),
      m_physicalDevice(other.m_physicalDevice),
      m_graphicsQueue(other.m_graphicsQueue),
      m_presentQueue(other.m_presentQueue),
      m_commandPool(other.m_commandPool),
      m_queueFamilies(other.m_queueFamilies),
      m_ownsDevice(other.m_ownsDevice) {
   other.m_device = VK_NULL_HANDLE;
   other.m_physicalDevice = VK_NULL_HANDLE;
   other.m_graphicsQueue = VK_NULL_HANDLE;
   other.m_presentQueue = VK_NULL_HANDLE;
   other.m_commandPool = VK_NULL_HANDLE;
   other.m_queueFamilies = {};
   other.m_ownsDevice = false;
}

VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other) noexcept {
   if (this != &other) {
      if (m_commandPool != VK_NULL_HANDLE) {
         vkDestroyCommandPool(m_device, m_commandPool, nullptr);
      }
      if (m_allocator != VK_NULL_HANDLE) {
         vmaDestroyAllocator(m_allocator);
      }
      if (m_ownsDevice) {
         vkb::destroy_device(m_vkbDevice);
      }
      m_vkbDevice = std::move(other.m_vkbDevice);
      m_device = other.m_device;
      m_physicalDevice = other.m_physicalDevice;
      m_graphicsQueue = other.m_graphicsQueue;
      m_presentQueue = other.m_presentQueue;
      m_commandPool = other.m_commandPool;
      m_queueFamilies = other.m_queueFamilies;
      m_ownsDevice = other.m_ownsDevice;
      other.m_device = VK_NULL_HANDLE;
      other.m_physicalDevice = VK_NULL_HANDLE;
      other.m_graphicsQueue = VK_NULL_HANDLE;
      other.m_presentQueue = VK_NULL_HANDLE;
      other.m_commandPool = VK_NULL_HANDLE;
      other.m_queueFamilies = {};
      other.m_ownsDevice = false;
   }
   return *this;
}

void VulkanDevice::CreateCommandPool() {
   VkCommandPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   poolInfo.queueFamilyIndex = m_queueFamilies.graphicsFamily.value();
   if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create command pool.");
   }
}

void VulkanDevice::CreateAllocator(const VulkanInstance& instance,
                                   const VkPhysicalDeviceBufferDeviceAddressFeatures& features) {
   VmaAllocatorCreateInfo allocatorInfo{};
   allocatorInfo.physicalDevice = m_physicalDevice;
   allocatorInfo.device = m_device;
   allocatorInfo.instance = instance.Get();
   // Enable buffer device address only if supported
   if (features.bufferDeviceAddress) {
      allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
   } else {
      allocatorInfo.flags = 0;
   }
   if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create VMA allocator");
   }
}

const VkDevice& VulkanDevice::Get() const { return m_device; }

const VkPhysicalDevice& VulkanDevice::GetPhysicalDevice() const { return m_physicalDevice; }

const VkQueue& VulkanDevice::GetGraphicsQueue() const { return m_graphicsQueue; }

const VkQueue& VulkanDevice::GetPresentQueue() const { return m_presentQueue; }

const VkCommandPool& VulkanDevice::GetCommandPool() const { return m_commandPool; }

const QueueFamilyIndices& VulkanDevice::GetQueueFamilies() const { return m_queueFamilies; }

const VmaAllocator& VulkanDevice::GetAllocator() const { return m_allocator; }

uint32_t VulkanDevice::GetGraphicsQueueFamily() const {
   return m_queueFamilies.graphicsFamily.value();
}

uint32_t VulkanDevice::GetPresentQueueFamily() const {
   return m_queueFamilies.presentFamily.value();
}
