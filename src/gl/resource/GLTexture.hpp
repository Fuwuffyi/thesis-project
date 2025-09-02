#pragma once

#include "../../core/resource/ITexture.hpp"

#include <glad/gl.h>
#include <string>

class GLTexture : public ITexture {
  public:
   GLTexture(const CreateInfo& info);
   GLTexture(const std::string& filepath, const bool generateMipmaps, const bool sRGB);
   GLTexture(const uint32_t width, const uint32_t height, const Format format,
             const bool isDepth = false, const uint32_t samples = 1);
   ~GLTexture();

   GLTexture(const GLTexture&) = delete;
   GLTexture& operator=(const GLTexture&) = delete;
   GLTexture(GLTexture&& other) noexcept;
   GLTexture& operator=(GLTexture&& other) noexcept;

   ResourceType GetType() const override;
   size_t GetMemoryUsage() const override;
   bool IsValid() const override;

   uint32_t GetWidth() const override;
   uint32_t GetHeight() const override;
   uint32_t GetDepth() const override;
   Format GetFormat() const override;
   void Bind(uint32_t unit = 0) const override;
   void* GetNativeHandle() const override;

   GLuint GetId() const;

  private:
   void CreateStorage();
   GLenum ConvertFormat(const Format format) const;
   GLenum ConvertTarget() const;

  private:
   GLuint m_id;
   uint32_t m_width;
   uint32_t m_height;
   uint32_t m_depth;
   Format m_format;
   bool m_isDepth;
   uint32_t m_samples;
};
