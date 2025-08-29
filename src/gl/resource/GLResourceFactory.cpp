#include "GLResourceFactory.hpp"

#include "../../core/resource/MeshLoader.hpp"
#include "../resource/GLMesh.hpp"
#include "../resource/GLTexture.hpp"

std::unique_ptr<ITexture> GLResourceFactory::CreateTexture(const ITexture::CreateInfo& info) {
   return std::make_unique<GLTexture>(info);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateTextureFromFile(const std::string_view filepath, const bool generateMipmaps, const bool sRGB) {
   return std::make_unique<GLTexture>(std::string{filepath}, generateMipmaps, sRGB);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateDepthTexture(const uint32_t width, const uint32_t height, const ITexture::Format format) {
   return std::make_unique<GLTexture>(width, height, format, true);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateRenderTarget(const uint32_t width, const uint32_t height, const ITexture::Format format, const uint32_t samples) {
   return std::make_unique<GLTexture>(width, height, format, false, samples);
}

std::unique_ptr<IMesh> GLResourceFactory::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
   return std::make_unique<GLMesh>(vertices, indices);
}

std::unique_ptr<IMesh> GLResourceFactory::CreateMeshFromFile(const std::string_view filepath) {
   const MeshLoader::MeshData meshData = MeshLoader::LoadMesh(filepath);
   const auto values = meshData.GetCombinedData();
   return std::make_unique<GLMesh>(values.first, values.second);
}

