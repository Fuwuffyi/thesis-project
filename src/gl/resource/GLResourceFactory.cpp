#include "GLResourceFactory.hpp"

#include "gl/resource/GLMesh.hpp"
#include "gl/resource/GLTexture.hpp"

std::unique_ptr<ITexture> GLResourceFactory::CreateTexture(const ITexture::CreateInfo& info) {
   return std::make_unique<GLTexture>(info);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateTextureFromFile(const std::string& filepath, const bool generateMipmaps, const bool sRGB) {
   return std::make_unique<GLTexture>(filepath, generateMipmaps, sRGB);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateDepthTexture(const uint32_t width, const uint32_t height, const ITexture::Format format) {
   return std::make_unique<GLTexture>(width, height, format, true);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateRenderTarget(const uint32_t width, const uint32_t height, const ITexture::Format format, const uint32_t samples) {
   return std::make_unique<GLTexture>(width, height, format, false, samples);
}

std::unique_ptr<IMesh> GLResourceFactory::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) {
   return std::make_unique<GLMesh>(vertices, indices);
}

std::unique_ptr<IMesh> GLResourceFactory::CreateMeshFromFile(const std::string& filepath) {
   // TODO: Implement mesh loading from file (e.g., OBJ, glTF)
   return nullptr;
}

