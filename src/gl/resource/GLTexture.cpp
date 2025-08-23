#include "GLTexture.hpp"

#include <stb_image.h>

GLTexture::GLTexture(const CreateInfo& info)
   :
   m_id(0),
   m_width(info.width),
   m_height(info.height),
   m_depth(info.depth),
   m_format(info.format),
   m_isDepth(false),
   m_samples(info.samples)
{
   glGenTextures(1, &m_id);
   CreateStorage();
}

GLTexture::GLTexture(const std::string& filepath, bool generateMipmaps, bool sRGB)
:
   m_id(0),
   m_width(0),
   m_height(0),
   m_depth(1),
   m_format(Format::RGBA8),
   m_isDepth(false),
   m_samples(1)

{
   glGenTextures(1, &m_id);

   int width, height, channels;
   stbi_set_flip_vertically_on_load(true);
   unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

   if (!data) {
      return;
   }

   m_width = static_cast<uint32_t>(width);
   m_height = static_cast<uint32_t>(height);
   m_depth = 1;
   m_format = sRGB ? Format::SRGB8_ALPHA8 : Format::RGBA8;

   glBindTexture(GL_TEXTURE_2D, m_id);

   GLenum internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
   GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;

   glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

   if (generateMipmaps) {
      glGenerateMipmap(GL_TEXTURE_2D);
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   stbi_image_free(data);
}

GLTexture::GLTexture(uint32_t width, uint32_t height, Format format, bool isDepth, uint32_t samples)
: m_width(width), m_height(height), m_depth(1), m_format(format), m_isDepth(isDepth), m_samples(samples) {
   glGenTextures(1, &m_id);
   CreateStorage();
}

GLTexture::~GLTexture() {
   if (m_id != 0) {
      glDeleteTextures(1, &m_id);
   }
}

GLTexture::GLTexture(GLTexture&& other) noexcept
   : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height),
   m_depth(other.m_depth), m_format(other.m_format), m_isDepth(other.m_isDepth), m_samples(other.m_samples) {
   other.m_id = 0;
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
   if (this != &other) {
      if (m_id != 0) {
         glDeleteTextures(1, &m_id);
      }

      m_id = other.m_id;
      m_width = other.m_width;
      m_height = other.m_height;
      m_depth = other.m_depth;
      m_format = other.m_format;
      m_isDepth = other.m_isDepth;
      m_samples = other.m_samples;

      other.m_id = 0;
   }
   return *this;
}

ResourceType GLTexture::GetType() const {
   if (m_isDepth) return ResourceType::DepthTexture;
   return ResourceType::Texture2D;
}

size_t GLTexture::GetMemoryUsage() const {
   size_t bytesPerPixel = 4; // Default RGBA8
   switch (m_format) {
      case Format::R8: bytesPerPixel = 1; break;
      case Format::RG8: bytesPerPixel = 2; break;
      case Format::RGB8: bytesPerPixel = 3; break;
      case Format::RGBA8: 
      case Format::SRGB8_ALPHA8: bytesPerPixel = 4; break;
      case Format::RGBA16F: bytesPerPixel = 8; break;
      case Format::RGBA32F: bytesPerPixel = 16; break;
      case Format::Depth24: bytesPerPixel = 3; break;
      case Format::Depth32F: bytesPerPixel = 4; break;
   }
   return m_width * m_height * m_depth * bytesPerPixel * m_samples;
}

bool GLTexture::IsValid() const {
   return m_id != 0;
}

void GLTexture::Bind(uint32_t unit) const {
   glActiveTexture(GL_TEXTURE0 + unit);
   glBindTexture(ConvertTarget(), m_id);
}

void* GLTexture::GetNativeHandle() const {
   return reinterpret_cast<void*>(static_cast<uintptr_t>(m_id));
}

void GLTexture::CreateStorage() {
   GLenum target = ConvertTarget();
   GLenum internalFormat = ConvertFormat(m_format);

   glBindTexture(target, m_id);

   if (m_samples > 1) {
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, internalFormat, m_width, m_height, GL_TRUE);
   } else {
      glTexImage2D(target, 0, internalFormat, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   }
}

GLenum GLTexture::ConvertFormat(Format format) const {
   switch (format) {
      case Format::R8: return GL_R8;
      case Format::RG8: return GL_RG8;
      case Format::RGB8: return GL_RGB8;
      case Format::RGBA8: return GL_RGBA8;
      case Format::RGBA16F: return GL_RGBA16F;
      case Format::RGBA32F: return GL_RGBA32F;
      case Format::SRGB8_ALPHA8: return GL_SRGB8_ALPHA8;
      case Format::Depth24: return GL_DEPTH_COMPONENT24;
      case Format::Depth32F: return GL_DEPTH_COMPONENT32F;
      default: return GL_RGBA8;
   }
}

GLenum GLTexture::ConvertTarget() const {
   if (m_samples > 1) return GL_TEXTURE_2D_MULTISAMPLE;
   return GL_TEXTURE_2D;
}

