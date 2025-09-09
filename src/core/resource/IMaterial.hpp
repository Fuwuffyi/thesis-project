#pragma once

#include "core/resource/IResource.hpp"
#include "core/resource/ITexture.hpp"

#include <glm/glm.hpp>

#include <string>
#include <string_view>
#include <variant>
#include <cstdint>

class ResourceManager;

using MaterialParam = std::variant<float, int32_t, uint32_t, glm::vec2, glm::vec3, glm::vec4,
                                   glm::mat2, glm::mat3, glm::mat4>;

struct TextureBinding final {
   TextureHandle texture{};
   uint32_t bindingSlot{};
   std::string samplerName;
};

class IMaterial : public IResource {
  public:
   ~IMaterial() override = default;

   // Parameter management
   virtual void SetParameter(const std::string_view name, const MaterialParam& value) = 0;
   [[nodiscard]] virtual MaterialParam GetParameter(const std::string_view name) const = 0;
   [[nodiscard]] virtual bool HasParameter(const std::string_view name) const noexcept = 0;

   // Texture management
   virtual void SetTexture(const std::string_view name, const TextureHandle texture) = 0;
   [[nodiscard]] virtual TextureHandle GetTexture(const std::string_view name) const = 0;
   [[nodiscard]] virtual bool HasTexture(const std::string_view name) const noexcept = 0;

   // Binding
   virtual void Bind(const uint32_t bindingPoint, const ResourceManager& resourceManager) = 0;
   virtual void UpdateUBO() = 0;

   // Material template info
   [[nodiscard]] virtual std::string_view GetTemplateName() const noexcept = 0;
   [[nodiscard]] virtual void* GetNativeHandle() const noexcept = 0;
};

using MaterialHandle = ResourceHandle<IMaterial>;
