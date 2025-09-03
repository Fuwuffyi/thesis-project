#pragma once

#include "core/resource/IResource.hpp"
#include "core/resource/ITexture.hpp"

#include <glm/glm.hpp>

#include <string>
#include <variant>

using MaterialParam =
   std::variant<float, int32_t, uint32_t, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

struct TextureBinding {
   TextureHandle texture;
   uint32_t bindingSlot;
   std::string samplerName;
};

class IMaterial : public IResource {
  public:
   virtual ~IMaterial() = default;

   // Parameter management
   virtual void SetParameter(const std::string& name, const MaterialParam& value) = 0;
   virtual MaterialParam GetParameter(const std::string& name) const = 0;
   virtual bool HasParameter(const std::string& name) const = 0;

   // Texture management
   virtual void SetTexture(const std::string& name, const TextureHandle& texture) = 0;
   virtual TextureHandle GetTexture(const std::string& name) const = 0;
   virtual bool HasTexture(const std::string& name) const = 0;

   // Binding
   virtual void Bind(const uint32_t bindingPoint) = 0;
   virtual void UpdateUBO() = 0;

   // Material template info
   virtual const std::string& GetTemplateName() const = 0;
   virtual void* GetNativeHandle() const = 0;
};

using MaterialHandle = ResourceHandle<IMaterial>;
