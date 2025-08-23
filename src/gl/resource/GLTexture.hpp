#pragma once

#include "../../core/resource/ITexture.hpp"

#include <glad/gl.h>
#include <string>

class GLTexture : public ITexture {
public:
   GLTexture(const CreateInfo& info);
   GLTexture(const std::string& filepath, bool generateMipmaps, bool sRGB);
   GLTexture(uint32_t width, uint32_t height, Format format, bool isDepth = false, uint32_t samples = 1);
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
   GLenum ConvertFormat(Format format) const;
   GLenum ConvertTarget() const;

   GLuint m_id;
   uint32_t m_width;
   uint32_t m_height;
   uint32_t m_depth;
   Format m_format;
   bool m_isDepth;
   uint32_t m_samples;
};

