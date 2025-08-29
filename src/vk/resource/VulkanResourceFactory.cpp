#include "VulkanResourceFactory.hpp"

#include "../resource/VulkanMesh.hpp"
#include "../resource/VulkanTexture.hpp"

#include <memory>

VulkanResourceFactory::VulkanResourceFactory(const VulkanDevice &device)
   :
   m_device(&device)
{}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTexture(const ITexture::CreateInfo& info) {
   return std::make_unique<VulkanTexture>(*m_device, info);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromFile(const std::string_view filepath, const bool generateMipmaps, const bool sRGB) {
   return std::make_unique<VulkanTexture>(*m_device, std::string{filepath}, generateMipmaps, sRGB);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateDepthTexture(const uint32_t width, const uint32_t height, const ITexture::Format format) {
   return std::make_unique<VulkanTexture>(*m_device, width, height, format, true);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateRenderTarget(const uint32_t width, const uint32_t height, const ITexture::Format format, const uint32_t samples) {
   return std::make_unique<VulkanTexture>(*m_device, width, height, format, false, samples);
}

std::unique_ptr<IMesh> VulkanResourceFactory::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
   return std::make_unique<VulkanMesh>(vertices, indices, *m_device);
}

