#include "VulkanResourceFactory.hpp"

#include "vk/resource/VulkanMesh.hpp"
#include "vk/resource/VulkanTexture.hpp"

#include <memory>

VulkanResourceFactory::VulkanResourceFactory(const VulkanDevice &device)
   :
   m_device(&device)
{}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTexture(const ITexture::CreateInfo& info) {
   // return std::make_unique<VulkanTexture>(*m_device, info);
   return nullptr;
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromFile(const std::string& filepath, bool generateMipmaps, bool sRGB) {
   // return std::make_unique<VulkanTexture>(*m_device, filepath, generateMipmaps, sRGB);
   return nullptr;
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateDepthTexture(uint32_t width, uint32_t height, ITexture::Format format) {
   //return std::make_unique<VulkanTexture>(*m_device, width, height, format, true);
   return nullptr;
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateRenderTarget(uint32_t width, uint32_t height, ITexture::Format format, uint32_t samples) {
   // return std::make_unique<VulkanTexture>(*m_device, width, height, format, false, samples);
   return nullptr;
}

std::unique_ptr<IMesh> VulkanResourceFactory::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) {
   return std::make_unique<VulkanMesh>(vertices, indices, *m_device);
}

std::unique_ptr<IMesh> VulkanResourceFactory::CreateMeshFromFile(const std::string& filepath) {
   // TODO: Implement mesh loading from file
   return nullptr;
}

