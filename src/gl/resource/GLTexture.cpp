#include "GLTexture.hpp"

#include <stb_image.h>

GLTexture::GLTexture(const CreateInfo& info)
    : m_id(0),
      m_width(info.width),
      m_height(info.height),
      m_depth(info.depth),
      m_format(info.format),
      m_isDepth(false),
      m_samples(info.samples) {
   glGenTextures(1, &m_id);
   CreateStorage();
}

GLTexture::GLTexture(const std::string& filepath, const bool generateMipmaps, const bool sRGB)
    : m_id(0),
      m_width(0),
      m_height(0),
      m_depth(1),
      m_format(Format::RGBA8),
      m_isDepth(false),
      m_samples(1)

{
   glGenTextures(1, &m_id);
   int32_t width, height, channels;
   stbi_set_flip_vertically_on_load(true);
   uint8_t* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
   if (!data) {
      glDeleteTextures(1, &m_id);
      m_id = 0;
      return;
   }
   m_width = static_cast<uint32_t>(width);
   m_height = static_cast<uint32_t>(height);
   m_depth = 1;
   GLenum format = GL_RGBA;
   GLenum internal = GL_RGBA8;
   switch (channels) {
      case 1:
         format = GL_RED;
         internal = GL_R8;
         m_format = Format::R8;
         break;
      case 2:
         format = GL_RG;
         internal = GL_RG8;
         m_format = Format::RG8;
         break;
      case 3:
         format = GL_RGB;
         internal = sRGB ? GL_SRGB8 : GL_RGB8;
         m_format = Format::RGB8;
         break;
      case 4:
         format = GL_RGBA;
         internal = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
         m_format = sRGB ? Format::SRGB8_ALPHA8 : Format::RGBA8;
         break;
      default:
         // Unexpected channel count, force RGBA
         format = GL_RGBA;
         internal = GL_RGBA8;
         m_format = Format::RGBA8;
         break;
   }
   glBindTexture(GL_TEXTURE_2D, m_id);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, format, GL_UNSIGNED_BYTE, data);
   if (generateMipmaps) {
      glGenerateMipmap(GL_TEXTURE_2D);
   }
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                   generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

GLTexture::~GLTexture() {
   if (m_id != 0) {
      glDeleteTextures(1, &m_id);
   }
}

GLTexture::GLTexture(GLTexture&& other) noexcept
    : m_id(other.m_id),
      m_width(other.m_width),
      m_height(other.m_height),
      m_depth(other.m_depth),
      m_format(other.m_format),
      m_isDepth(other.m_isDepth),
      m_samples(other.m_samples) {
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

ResourceType GLTexture::GetType() const { return ResourceType::Texture; }

size_t GLTexture::GetMemoryUsage() const {
   size_t bytesPerPixel = 4;
   switch (m_format) {
      case Format::R8:
         bytesPerPixel = 1;
         break;
      case Format::RG8:
         bytesPerPixel = 2;
         break;
      case Format::RGB8:
         bytesPerPixel = 3;
         break;
      case Format::RGBA8:
         break;
      case Format::SRGB8_ALPHA8:
         bytesPerPixel = 4;
         break;
      case Format::RGBA16F:
         bytesPerPixel = 8;
         break;
      case Format::RGBA32F:
         bytesPerPixel = 16;
         break;
      case Format::Depth24:
         bytesPerPixel = 3;
         break;
      case Format::Depth32F:
         bytesPerPixel = 4;
         break;
   }
   return m_width * m_height * m_depth * bytesPerPixel * m_samples;
}

bool GLTexture::IsValid() const { return m_id != 0; }

void GLTexture::Bind(const uint32_t unit) const {
   glActiveTexture(GL_TEXTURE0 + unit);
   glBindTexture(ConvertTarget(), m_id);
}

void* GLTexture::GetNativeHandle() const {
   return reinterpret_cast<void*>(static_cast<uintptr_t>(m_id));
}

void GLTexture::CreateStorage() {
   const GLenum target = ConvertTarget();
   const GLenum internalFormat = ConvertFormat(m_format);
   glBindTexture(target, m_id);
   if (m_samples > 1) {
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, internalFormat, m_width,
                              m_height, GL_TRUE);
      return;
   }
   GLenum externalFormat = GL_RGBA;
   GLenum externalType = GL_UNSIGNED_BYTE;
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
         case Format::RGBA8:
         case Format::SRGB8_ALPHA8:
         case Format::RGBA16F:
         case Format::RGBA32F:
            externalFormat = GL_RGBA;
            break;
         default:
            externalFormat = GL_RGBA;
            break;
      }
      if (m_format == Format::RGBA16F || m_format == Format::RGBA32F) {
         externalType = GL_FLOAT;
      }
   }
   glTexImage2D(target, 0, internalFormat, m_width, m_height, 0, externalFormat, externalType,
                nullptr);
   glTexParameteri(target, GL_TEXTURE_MIN_FILTER, m_isDepth ? GL_NEAREST : GL_LINEAR);
   glTexParameteri(target, GL_TEXTURE_MAG_FILTER, m_isDepth ? GL_NEAREST : GL_LINEAR);
   glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

GLenum GLTexture::ConvertFormat(Format format) const {
   switch (format) {
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

GLenum GLTexture::ConvertTarget() const {
   if (m_samples > 1)
      return GL_TEXTURE_2D_MULTISAMPLE;
   return GL_TEXTURE_2D;
}

uint32_t GLTexture::GetWidth() const { return m_width; }

uint32_t GLTexture::GetHeight() const { return m_height; }

uint32_t GLTexture::GetDepth() const { return m_depth; }

ITexture::Format GLTexture::GetFormat() const { return m_format; }

GLuint GLTexture::GetId() const { return m_id; }
