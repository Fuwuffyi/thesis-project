#include "gl/resource/GLTexture.hpp"

#include <glad/gl.h>
#include <stb_image.h>

#include <utility>
#include <cassert>

static constexpr size_t BytesPerPixelForFormat(const ITexture::Format fmt) noexcept {
   switch (fmt) {
      case ITexture::Format::R8:
         return 1;
      case ITexture::Format::RG8:
         return 2;
      case ITexture::Format::RGB8:
         return 3;
      case ITexture::Format::RGBA8:
         return 4;
      case ITexture::Format::SRGB8_ALPHA8:
         return 4;
      case ITexture::Format::RGBA16F:
         return 8;
      case ITexture::Format::RGBA32F:
         return 16;
      case ITexture::Format::Depth24:
         return 3;
      case ITexture::Format::Depth32F:
         return 4;
   }
   return 4;
}

GLTexture::GLTexture(const CreateInfo& info)
    : m_width(info.width),
      m_height(info.height),
      m_depth(info.depth),
      m_format(info.format),
      m_isDepth(false),
      m_samples(info.samples) {
   glGenTextures(1, &m_id);
   CreateStorage();
}

GLTexture::GLTexture(const std::string& filepath, const bool generateMipmaps, const bool sRGB)
    : m_depth(1), m_format(Format::RGBA8), m_isDepth(false), m_samples(1) {
   glGenTextures(1, &m_id);
   int32_t w = 0, h = 0, channels = 0;
   unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &channels, 0);
   if (!data) {
      glDeleteTextures(1, &m_id);
      m_id = 0;
      return;
   }
   m_width = static_cast<uint32_t>(w);
   m_height = static_cast<uint32_t>(h);
   uint32_t externalFormat = GL_RGBA;
   uint32_t internalFormat = GL_RGBA8;
   switch (channels) {
      case 1:
         externalFormat = GL_RED;
         internalFormat = GL_R8;
         m_format = Format::R8;
         break;
      case 2:
         externalFormat = GL_RG;
         internalFormat = GL_RG8;
         m_format = Format::RG8;
         break;
      case 3:
         externalFormat = GL_RGB;
         internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
         m_format = Format::RGB8;
         break;
      case 4:
         externalFormat = GL_RGBA;
         internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
         m_format = sRGB ? Format::SRGB8_ALPHA8 : Format::RGBA8;
         break;
      default:
         externalFormat = GL_RGBA;
         internalFormat = GL_RGBA8;
         m_format = Format::RGBA8;
         break;
   }
   const uint32_t target = GL_TEXTURE_2D;
   glBindTexture(target, m_id);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexStorage2D(
      target,
      1u + (generateMipmaps ? static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) : 0u),
      internalFormat, m_width, m_height);
   glTexSubImage2D(target, 0, 0, 0, m_width, m_height, externalFormat, GL_UNSIGNED_BYTE, data);
   if (generateMipmaps) {
      glGenerateMipmap(target);
   }
   // Filtering and wrapping
   glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                   generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
   glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
   stbi_image_free(data);
}

GLTexture::GLTexture(const uint32_t width, const uint32_t height, const Format format,
                     const bool isDepth, const uint32_t samples)
    : m_width(width),
      m_height(height),
      m_depth(1),
      m_format(format),
      m_isDepth(isDepth),
      m_samples(samples) {
   glGenTextures(1, &m_id);
   CreateStorage();
}

GLTexture::GLTexture(const Format format, const glm::vec4& color)
    : m_width(1), m_height(1), m_depth(1), m_format(format), m_isDepth(false), m_samples(1) {
   glGenTextures(1, &m_id);
   glBindTexture(GL_TEXTURE_2D, m_id);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   const unsigned char rgba[] = {
      static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f),
      static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f),
      static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f),
      static_cast<unsigned char>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f)};
   uint32_t externalFormat = GL_RGBA;
   switch (format) {
      case Format::R8:
         externalFormat = GL_RED;
         break;
      case Format::RG8:
         externalFormat = GL_RG;
         break;
      case Format::RGB8:
         externalFormat = GL_RGB;
         break;
      default:
         externalFormat = GL_RGBA;
         break;
   }

   const uint32_t internal = ConvertFormatInternal(format);
   // Immutable storage preferred
   glTexStorage2D(GL_TEXTURE_2D, 1, internal, 1, 1);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, externalFormat, GL_UNSIGNED_BYTE, rgba);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

GLTexture::~GLTexture() noexcept {
   if (m_id != 0) {
      glDeleteTextures(1, &m_id);
   }
}

GLTexture::GLTexture(GLTexture&& other) noexcept
    : m_id(std::exchange(other.m_id, 0)),
      m_width(other.m_width),
      m_height(other.m_height),
      m_depth(other.m_depth),
      m_format(other.m_format),
      m_isDepth(other.m_isDepth),
      m_samples(other.m_samples) {}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
   if (this == &other)
      return *this;
   if (m_id != 0) {
      glDeleteTextures(1, &m_id);
   }
   m_id = std::exchange(other.m_id, 0);
   m_width = other.m_width;
   m_height = other.m_height;
   m_depth = other.m_depth;
   m_format = other.m_format;
   m_isDepth = other.m_isDepth;
   m_samples = other.m_samples;
   return *this;
}

ResourceType GLTexture::GetType() const noexcept { return ResourceType::Texture; }

size_t GLTexture::GetMemoryUsage() const noexcept {
   return static_cast<size_t>(m_width) * m_height * m_depth * BytesPerPixelForFormat(m_format) *
          std::max<uint32_t>(1, m_samples);
}

bool GLTexture::IsValid() const noexcept { return m_id != 0; }

void GLTexture::Bind(const uint32_t unit) const noexcept {
   glActiveTexture(GL_TEXTURE0 + unit);
   glBindTexture(ConvertTarget(), m_id);
}

void GLTexture::CreateStorage() {
   assert(m_id != 0);
   const uint32_t target = ConvertTarget();
   const uint32_t internal = ConvertFormatInternal(m_format);
   glBindTexture(target, m_id);
   if (m_samples > 1) {
      glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, internal, m_width, m_height,
                                GL_TRUE);
      return;
   }
   uint32_t externalFormat = GL_RGBA;
   uint32_t externalType = GL_UNSIGNED_BYTE;
   if (m_isDepth) {
      externalFormat = GL_DEPTH_COMPONENT;
      externalType = (m_format == Format::Depth32F) ? GL_FLOAT : GL_UNSIGNED_INT;
   } else {
      switch (m_format) {
         case Format::R8:
            externalFormat = GL_RED;
            break;
         case Format::RG8:
            externalFormat = GL_RG;
            break;
         case Format::RGB8:
            externalFormat = GL_RGB;
            break;
         default:
            externalFormat = GL_RGBA;
            break;
      }
      if (m_format == Format::RGBA16F || m_format == Format::RGBA32F) {
         externalType = GL_FLOAT;
      }
   }
   glTexStorage2D(target, 1, internal, m_width, m_height);
   glTexParameteri(target, GL_TEXTURE_MIN_FILTER, m_isDepth ? GL_NEAREST : GL_LINEAR);
   glTexParameteri(target, GL_TEXTURE_MAG_FILTER, m_isDepth ? GL_NEAREST : GL_LINEAR);
   glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

uint32_t GLTexture::ConvertFormatInternal(const Format fmt) const noexcept {
   switch (fmt) {
      case Format::R8:
         return GL_R8;
      case Format::RG8:
         return GL_RG8;
      case Format::RGB8:
         return GL_RGB8;
      case Format::RGBA8:
         return GL_RGBA8;
      case Format::RGBA16F:
         return GL_RGBA16F;
      case Format::RGBA32F:
         return GL_RGBA32F;
      case Format::SRGB8_ALPHA8:
         return GL_SRGB8_ALPHA8;
      case Format::Depth24:
         return GL_DEPTH_COMPONENT24;
      case Format::Depth32F:
         return GL_DEPTH_COMPONENT32F;
      default:
         return GL_RGBA8;
   }
}

uint32_t GLTexture::ConvertTarget() const noexcept {
   return (m_samples > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
}
