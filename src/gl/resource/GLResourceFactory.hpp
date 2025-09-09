#pragma once

#include "core/resource/IResourceFactory.hpp"

class GLResourceFactory final : public IResourceFactory {
  public:
   GLResourceFactory() = default;
   ~GLResourceFactory() override = default;

   std::unique_ptr<ITexture> CreateTexture(const ITexture::CreateInfo& info) override;
   std::unique_ptr<ITexture> CreateTextureColor(const ITexture::Format format,
                                                const glm::vec4& color) override;
   std::unique_ptr<ITexture> CreateTextureFromFile(const std::string_view filepath,
                                                   const bool generateMipmaps,
                                                   const bool sRGB) override;
   std::unique_ptr<ITexture> CreateDepthTexture(const uint32_t width, const uint32_t height,
                                                const ITexture::Format format) override;
   std::unique_ptr<ITexture> CreateRenderTarget(const uint32_t width, const uint32_t height,
                                                const ITexture::Format format,
                                                const uint32_t samples) override;

   std::unique_ptr<IMaterial> CreateMaterial(const MaterialTemplate& matTemplate) override;
   std::unique_ptr<IMesh> CreateMesh(const std::vector<Vertex>& vertices,
                                     const std::vector<uint32_t>& indices) override;
};
