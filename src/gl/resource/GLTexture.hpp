#pragma once

#include "core/resource/ITexture.hpp"

#include <glm/glm.hpp>
#include <string>

class GLTexture final : public ITexture {
  public:
   explicit GLTexture(const CreateInfo& info);
   explicit GLTexture(const std::string& filepath, const bool generateMipmaps = true,
                      const bool sRGB = true);
   GLTexture(const uint32_t width, const uint32_t height, const Format format,
             const bool isDepth = false, const uint32_t samples = 1);
   GLTexture(const Format format, const glm::vec4& color);
   ~GLTexture() noexcept override;

   GLTexture(const GLTexture&) = delete;
   GLTexture& operator=(const GLTexture&) = delete;
   GLTexture(GLTexture&& other) noexcept;
   GLTexture& operator=(GLTexture&& other) noexcept;

   // IResource
   [[nodiscard]] ResourceType GetType() const noexcept override;
   [[nodiscard]] size_t GetMemoryUsage() const noexcept override;
   [[nodiscard]] bool IsValid() const noexcept override;

   // ITexture
   [[nodiscard]] constexpr uint32_t GetWidth() const noexcept override { return m_width; }
   [[nodiscard]] constexpr uint32_t GetHeight() const noexcept override { return m_height; }
   [[nodiscard]] constexpr uint32_t GetDepth() const noexcept override { return m_depth; }
   [[nodiscard]] constexpr Format GetFormat() const noexcept override { return m_format; }
   void Bind(const uint32_t unit = 0) const noexcept override;
   [[nodiscard]] void* GetNativeHandle() const noexcept override;

   [[nodiscard]] constexpr uint32_t GetId() const noexcept { return m_id; }

  private:
   void CreateStorage();
   uint32_t ConvertFormatInternal(const Format format) const noexcept;
   uint32_t ConvertTarget() const noexcept;

  private:
   uint32_t m_id{0};
   uint32_t m_width{0};
   uint32_t m_height{0};
   uint32_t m_depth{1};
   Format m_format{Format::RGBA8};
   bool m_isDepth{false};
   uint32_t m_samples{1};
};
