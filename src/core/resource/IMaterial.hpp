#pragma once

#include "core/resource/IResource.hpp"
#include "core/resource/ITexture.hpp"

#include <glm/glm.hpp>
#include <string>
#include <variant>
#include <vector>

class IMaterial : public IResource {
public:
   using PropertyValue = std::variant<
   float,
   glm::vec2,
   glm::vec3,
   glm::vec4,
   int32_t,
   uint32_t,
   bool,
   TextureHandle
   >;

   enum class PropertyType {
      Float,
      Vec2,
      Vec3,
      Vec4,
      Int,
      UInt,
      Bool,
      Texture
   };

   struct PropertyInfo {
      PropertyType type;
      std::string name;
      PropertyValue defaultValue;
      std::string description;
   };

   virtual ~IMaterial() = default;

   // Property management
   virtual void SetProperty(const std::string& name, const PropertyValue& value) = 0;
   virtual PropertyValue GetProperty(const std::string& name) const = 0;
   virtual bool HasProperty(const std::string& name) const = 0;
   virtual void RemoveProperty(const std::string& name) = 0;

   // Texture slot management
   virtual void SetTexture(const std::string& slotName, TextureHandle texture) = 0;
   virtual TextureHandle GetTexture(const std::string& slotName) const = 0;
   virtual bool HasTexture(const std::string& slotName) const = 0;
   virtual void RemoveTexture(const std::string& slotName) = 0;

   // Shader binding
   virtual void Bind() const = 0;
   virtual void Unbind() const = 0;

   // Utility
   virtual std::vector<std::string> GetPropertyNames() const = 0;
   virtual std::vector<std::string> GetTextureSlotNames() const = 0;
   virtual const std::string& GetMaterialType() const = 0;
};

using MaterialHandle = ResourceHandle<IMaterial>;

