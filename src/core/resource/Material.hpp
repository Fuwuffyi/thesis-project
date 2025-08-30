#pragma once

#include "core/resource/MaterialTemplate.hpp"

class Material final : public IMaterial {
public:
   explicit Material(const MaterialTemplate& materialTemplate);
   Material(const std::string& typeName, const MaterialTemplate& materialTemplate);
   ~Material() = default;

   // IResource implementation
   ResourceType GetType() const override;
   size_t GetMemoryUsage() const override;
   bool IsValid() const override;

   // IMaterial implementation
   void SetProperty(const std::string& name, const PropertyValue& value) override;
   PropertyValue GetProperty(const std::string& name) const override;
   bool HasProperty(const std::string& name) const override;
   void RemoveProperty(const std::string& name) override;

   void SetTexture(const std::string& slotName, const TextureHandle texture) override;
   TextureHandle GetTexture(const std::string& slotName) const override;
   bool HasTexture(const std::string& slotName) const override;
   void RemoveTexture(const std::string& slotName) override;

   std::vector<std::string> GetPropertyNames() const override;
   std::vector<std::string> GetTextureSlotNames() const override;
   const std::string& GetMaterialType() const override;

   // Template access
   const MaterialTemplate& GetTemplate() const;

private:
   std::string m_typeName;
   const MaterialTemplate& m_template;
   std::unordered_map<std::string, PropertyValue> m_properties;
   std::unordered_map<std::string, TextureHandle> m_textures;
};

