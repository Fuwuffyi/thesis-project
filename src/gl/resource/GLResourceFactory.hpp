#pragma once

#include "../../core/resource/IResourceFactory.hpp"

class GLResourceFactory : public IResourceFactory {
public:
   GLResourceFactory() = default;
   ~GLResourceFactory() = default;

   std::unique_ptr<ITexture> CreateTexture(const ITexture::CreateInfo& info) override;
   std::unique_ptr<ITexture> CreateTextureFromFile(const std::string& filepath, bool generateMipmaps, bool sRGB) override;
   std::unique_ptr<ITexture> CreateDepthTexture(uint32_t width, uint32_t height, ITexture::Format format) override;
   std::unique_ptr<ITexture> CreateRenderTarget(uint32_t width, uint32_t height, ITexture::Format format, uint32_t samples) override;
   std::unique_ptr<IMesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) override;
   std::unique_ptr<IMesh> CreateMeshFromFile(const std::string& filepath) override;
};

