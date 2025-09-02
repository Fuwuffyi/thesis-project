#pragma once

#include <cstddef>

enum class ResourceType { Texture, Mesh, Material };

class IResource {
  public:
   virtual ~IResource() = default;
   virtual ResourceType GetType() const = 0;
   virtual size_t GetMemoryUsage() const = 0;
   virtual bool IsValid() const = 0;
};
