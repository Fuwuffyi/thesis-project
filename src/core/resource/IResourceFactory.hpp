#pragma once

#include "core/Vertex.hpp"

#include "core/resource/IMaterial.hpp"
#include "core/resource/ITexture.hpp"
#include "core/resource/IMesh.hpp"

#include "core/resource/MaterialTemplate.hpp"

#include <vector>
#include <memory>

class IResourceFactory {
  public:
   virtual ~IResourceFactory() = default;
   // Texture creation methods
   virtual std::unique_ptr<ITexture> CreateTexture(const ITexture::CreateInfo& info) = 0;
   virtual std::unique_ptr<ITexture> CreateTextureFromFile(const std::string_view filepath,
                                                           const bool generateMipmaps = true,
                                                           const bool sRGB = true) = 0;
   virtual std::unique_ptr<ITexture> CreateDepthTexture(
      const uint32_t width, const uint32_t height,
      const ITexture::Format format = ITexture::Format::Depth24) = 0;
   virtual std::unique_ptr<ITexture> CreateRenderTarget(
      const uint32_t width, const uint32_t height,
      const ITexture::Format format = ITexture::Format::RGBA8, const uint32_t samples = 1) = 0;
   // Material creation methods
   virtual std::unique_ptr<IMaterial> CreateMaterial(const MaterialTemplate& matTemplate) = 0;
   // Mesh creation methods
   virtual std::unique_ptr<IMesh> CreateMesh(const std::vector<Vertex>& vertices,
                                             const std::vector<uint32_t>& indices) = 0;
};
