#include "VulkanInstance.hpp"

VulkanInstance::VulkanInstance() {
   vkb::InstanceBuilder builder;
   const auto inst_ret = builder.set_app_name("Thesis Project")
                      .set_app_version(1, 0, 0)
                      .set_engine_name("No Engine")
                      .set_engine_version(1, 0, 0)
                      .require_api_version(1, 4, 0)
#ifndef NDEBUG
                      .request_validation_layers(true)
#endif
                      .use_default_debug_messenger()
                      .build();
   if (!inst_ret) {
      throw std::runtime_error("Failed to create Vulkan instance: " + inst_ret.error().message());
   }
   m_vkbInstance = inst_ret.value();
   m_instance = m_vkbInstance.instance;
   m_ownsInstance = true;
}

VulkanInstance::~VulkanInstance() {
   if (m_ownsInstance) {
      vkb::destroy_instance(m_vkbInstance);
   }
}

VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept
    : m_vkbInstance(std::move(other.m_vkbInstance)),
      m_instance(other.m_instance),
      m_ownsInstance(other.m_ownsInstance) {
   other.m_instance = VK_NULL_HANDLE;
   other.m_ownsInstance = false;
}

VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept {
   if (this != &other) {
      if (m_ownsInstance) {
         vkb::destroy_instance(m_vkbInstance);
      }
      m_vkbInstance = std::move(other.m_vkbInstance);
      m_instance = other.m_instance;
      m_ownsInstance = other.m_ownsInstance;
      other.m_instance = VK_NULL_HANDLE;
      other.m_ownsInstance = false;
   }
   return *this;
}

VkInstance VulkanInstance::Get() const { return m_instance; }

const vkb::Instance& VulkanInstance::GetVkbInstance() const { return m_vkbInstance; }
