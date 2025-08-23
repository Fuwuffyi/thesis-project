#pragma once

#include <cstddef>

class ITexture;
class IMesh;

enum class ResourceType {
   Texture2D,
   TextureCube,
   Texture3D,
   DepthTexture,
   RenderTarget,
   Mesh,
   Buffer
};

class IResource {
public:
   virtual ~IResource() = default;
   virtual ResourceType GetType() const = 0;
   virtual size_t GetMemoryUsage() const = 0;
   virtual bool IsValid() const = 0;
};

