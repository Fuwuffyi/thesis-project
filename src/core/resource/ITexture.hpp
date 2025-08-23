#pragma once

#include "IResource.hpp"

#include <cstdint>

class ITexture : public IResource {
public:
   enum class Format {
      RGBA8,
      RGBA16F,
      RGBA32F,
      SRGB8_ALPHA8,
      Depth24,
      Depth32F,
      R8,
      RG8,
      RGB8
   };

   enum class FilterMode {
      Nearest,
      Linear,
      LinearMipmapLinear,
      NearestMipmapNearest
   };

   enum class WrapMode {
      Repeat,
      ClampToEdge,
      ClampToBorder,
      MirroredRepeat
   };

   struct CreateInfo {
      uint32_t width = 0;
      uint32_t height = 0;
      uint32_t depth = 1;
      Format format = Format::RGBA8;
      bool generateMipmaps = false;
      bool sRGB = false;
      uint32_t samples = 1;
      FilterMode minFilter = FilterMode::Linear;
      FilterMode magFilter = FilterMode::Linear;
      WrapMode wrapS = WrapMode::Repeat;
      WrapMode wrapT = WrapMode::Repeat;
      WrapMode wrapR = WrapMode::Repeat;
   };

   virtual ~ITexture() = default;
   virtual uint32_t GetWidth() const = 0;
   virtual uint32_t GetHeight() const = 0;
   virtual uint32_t GetDepth() const = 0;
   virtual Format GetFormat() const = 0;
   virtual void Bind(const uint32_t unit = 0) const = 0;
   virtual void* GetNativeHandle() const = 0;
};

