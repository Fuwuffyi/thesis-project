#pragma once

#include <cstddef>

enum class ResourceType { Texture, Mesh, Material };

class IResource {
  public:
   virtual ~IResource() = default;
   [[nodiscard]] virtual ResourceType GetType() const noexcept = 0;
   [[nodiscard]] virtual size_t GetMemoryUsage() const noexcept = 0;
   [[nodiscard]] virtual bool IsValid() const noexcept = 0;
};
