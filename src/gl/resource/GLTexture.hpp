#pragma once

#include <glad/gl.h>
#include <string>

class GLTexture {
public:
   enum class Target : GLenum {
      Tex2D       = GL_TEXTURE_2D,
      Tex3D       = GL_TEXTURE_3D,
      CubeMap     = GL_TEXTURE_CUBE_MAP,
      Tex2DArray  = GL_TEXTURE_2D_ARRAY
   };

   enum class Format : GLenum {
      RGBA8       = GL_RGBA8,
      RGBA16F     = GL_RGBA16F,
      SRGB8_ALPHA8= GL_SRGB8_ALPHA8,
      Depth24     = GL_DEPTH_COMPONENT24,
      Depth32F    = GL_DEPTH_COMPONENT32F
   };

   GLTexture(const Target target,
             const Format format,
             const uint32_t width,
             const uint32_t height,
             const uint32_t depth = 1,
             const uint32_t mipLevels = 1);

   GLTexture(const std::string& filepath, const bool generateMipmaps = true, const bool sRGB = true);

   ~GLTexture();

   GLTexture(const GLTexture&) = delete;
   GLTexture& operator=(const GLTexture&) = delete;
   GLTexture(GLTexture&& other) noexcept;
   GLTexture& operator=(GLTexture&& other) noexcept;

   GLuint Get() const;
   Target GetTarget() const;
   Format GetFormat() const;
   uint32_t GetWidth() const;
   uint32_t GetHeight() const;
   uint32_t GetDepth() const;
   uint32_t GetMipLevels() const;

   void BindUnit(const GLuint unit) const;

   void Upload2D(const uint32_t level, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
                 const GLenum format, const GLenum type, const void* pixels);

   void GenerateMipmaps();

   static uint32_t CalculateMipLevels(const uint32_t width, const uint32_t height);

private:
   void CreateStorage();

private:
   GLuint m_id = 0;
   Target m_target;
   Format m_format;
   uint32_t m_width;
   uint32_t m_height;
   uint32_t m_depth;
   uint32_t m_mipLevels;
};

