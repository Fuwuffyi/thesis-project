#include "GLTexture.hpp"

#include "stb_image.h"

#include <cmath>
#include <stdexcept>
#include <cassert>
#include <cstring>

static GLenum FormatToInternal(const GLTexture::Format fmt) {
   switch (fmt) {
      case GLTexture::Format::RGBA8:
         return GL_RGBA8;
      case GLTexture::Format::RGBA16F:
         return GL_RGBA16F;
      case GLTexture::Format::SRGB8_ALPHA8:
         return GL_SRGB8_ALPHA8;
      case GLTexture::Format::Depth24:
         return GL_DEPTH_COMPONENT24;
      case GLTexture::Format::Depth32F: 
         return GL_DEPTH_COMPONENT32F;
      default:
         return GL_RGBA8;
   }
}

static GLenum FormatToBase(const GLTexture::Format fmt) {
   switch (fmt) {
      case GLTexture::Format::RGBA8:
      case GLTexture::Format::RGBA16F:
      case GLTexture::Format::SRGB8_ALPHA8:
         return GL_RGBA;
      case GLTexture::Format::Depth24:
      case GLTexture::Format::Depth32F:
         return GL_DEPTH_COMPONENT;
      default:
         return GL_RGBA;
   }
}

GLTexture::GLTexture(const Target target,
                     const Format format,
                     const uint32_t width,
                     const uint32_t height,
                     const uint32_t depth,
                     const uint32_t mipLevels)
   :
   m_target(target),
   m_format(format),
   m_width(width),
   m_height(height),
   m_depth(depth),
   m_mipLevels(mipLevels)
{
   glCreateTextures(static_cast<GLenum>(m_target), 1, &m_id);
   if (m_id == 0) {
      throw std::runtime_error("glCreateTextures failed");
   }
   CreateStorage();
}

GLTexture::GLTexture(const std::string& filepath, const bool generateMipmaps, const bool sRGB) {
   int32_t w, h, comp;
   stbi_uc* data = stbi_load(filepath.c_str(), &w, &h, &comp, 4);
   if (!data) {
      throw std::runtime_error(std::string("Failed to load texture: ") + filepath);
   }
   m_target = Target::Tex2D;
   m_width = w;
   m_height = h;
   m_depth = 1;
   m_mipLevels = generateMipmaps ? CalculateMipLevels(w, h) : 1;
   m_format = sRGB ? Format::SRGB8_ALPHA8 : Format::RGBA8;
   glCreateTextures(static_cast<GLenum>(m_target), 1, &m_id);
   if (m_id == 0) {
      stbi_image_free(data);
      throw std::runtime_error("glCreateTextures failed.");
   }
   CreateStorage();
   const GLenum baseFormat = FormatToBase(m_format);
   glTextureSubImage2D(m_id, 0, 0, 0,
                       m_width, m_height, baseFormat, GL_UNSIGNED_BYTE, data);
   if (generateMipmaps) {
      glGenerateTextureMipmap(m_id);
   }
   stbi_image_free(data);
}

GLTexture::~GLTexture() {
   if (m_id != 0) {
      glDeleteTextures(1, &m_id);
      m_id = 0;
   }
}

GLTexture::GLTexture(GLTexture&& other) noexcept
   :
   m_id(other.m_id),
   m_target(other.m_target),
   m_format(other.m_format),
   m_width(other.m_width),
   m_height(other.m_height),
   m_depth(other.m_depth),
   m_mipLevels(other.m_mipLevels)
{
   other.m_id = 0;
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
   if (this != &other) {
      if (m_id != 0) {
         glDeleteTextures(1, &m_id);
      }
      m_id = other.m_id;
      m_target = other.m_target;
      m_format = other.m_format;
      m_width = other.m_width;
      m_height = other.m_height;
      m_depth = other.m_depth;
      m_mipLevels = other.m_mipLevels;
      other.m_id = 0;
   }
   return *this;
}

void GLTexture::CreateStorage() {
   const GLenum internal = FormatToInternal(m_format);
   switch (m_target) {
      case Target::Tex2D:
         glTextureStorage2D(m_id, m_mipLevels, internal, m_width, m_height);
         break;

      case Target::CubeMap:
         // Cube map faces are allocated automatically as 6 layers when using GL_TEXTURE_CUBE_MAP
         glTextureStorage2D(m_id, m_mipLevels, internal, m_width, m_height);
         break;

      case Target::Tex3D:
         glTextureStorage3D(m_id, m_mipLevels, internal, m_width, m_height, m_depth);
         break;

      case Target::Tex2DArray:
         glTextureStorage3D(m_id, m_mipLevels, internal, m_width, m_height, m_depth);
         break;

      default:
         throw std::runtime_error("Unsupported target.");
   }
}

void GLTexture::BindUnit(const GLuint unit) const {
   glBindTextureUnit(unit, m_id);
}

void GLTexture::Upload2D(const uint32_t level, const uint32_t x, const uint32_t y,
                         const uint32_t width, const uint32_t height,
                         const GLenum format, const GLenum type, const void* pixels)
{
   if (m_target == Target::Tex2D || m_target == Target::CubeMap) {
      glTextureSubImage2D(m_id, level, x, y, width, height, format, type, pixels);
   } else if (m_target == Target::Tex2DArray || m_target == Target::Tex3D) {
      throw std::runtime_error("Use a 3D/array upload function for Tex2DArray/Tex3D.");
   } else {
      throw std::runtime_error("Unsupported target.");
   }
}

void GLTexture::GenerateMipmaps() {
   if (m_id == 0) return;
   glGenerateTextureMipmap(m_id);
}

uint32_t GLTexture::CalculateMipLevels(const uint32_t width, uint32_t height) {
   const uint32_t larger = std::max(width, height);
   return 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(1u, larger))));
}

GLuint GLTexture::Get() const {
   return m_id;
}

GLTexture::Target GLTexture::GetTarget() const {
   return m_target;
}

GLTexture::Format GLTexture::GetFormat() const {
   return m_format;
}

uint32_t GLTexture::GetWidth() const {
   return m_width;
}

uint32_t GLTexture::GetHeight() const {
   return m_height;
}

uint32_t GLTexture::GetDepth() const {
   return m_depth;
}

uint32_t GLTexture::GetMipLevels() const {
   return m_mipLevels;
}

