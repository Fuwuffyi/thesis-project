#include "gl/resource/GLResourceFactory.hpp"

#include "gl/resource/GLTexture.hpp"
#include "gl/resource/GLMaterial.hpp"
#include "gl/resource/GLMesh.hpp"

std::unique_ptr<ITexture> GLResourceFactory::CreateTexture(const ITexture::CreateInfo& info) {
   return std::make_unique<GLTexture>(info);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateTextureColor(const ITexture::Format format,
                                             const glm::vec4& color) {
   return std::make_unique<GLTexture>(format, color);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateTextureFromFile(const std::string_view filepath,
                                                                   const bool generateMipmaps,
                                                                   const bool sRGB) {
   return std::make_unique<GLTexture>(std::string{filepath}, generateMipmaps, sRGB);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateDepthTexture(const uint32_t width,
                                                                const uint32_t height,
                                                                const ITexture::Format format) {
   return std::make_unique<GLTexture>(width, height, format, true);
}

std::unique_ptr<ITexture> GLResourceFactory::CreateRenderTarget(const uint32_t width,
                                                                const uint32_t height,
                                                                const ITexture::Format format,
                                                                const uint32_t samples) {
   return std::make_unique<GLTexture>(width, height, format, false, samples);
}

std::unique_ptr<IMaterial> GLResourceFactory::CreateMaterial(const MaterialTemplate& matTemplate) {
   return std::make_unique<GLMaterial>(matTemplate);
}

std::unique_ptr<IMesh> GLResourceFactory::CreateMesh(const std::vector<Vertex>& vertices,
                                                     const std::vector<uint32_t>& indices) {
   return std::make_unique<GLMesh>(vertices, indices);
}
