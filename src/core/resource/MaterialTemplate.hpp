#pragma once

#include "core/resource/IMaterial.hpp"

#include <functional>

class MaterialTemplate final {
public:
   struct TextureSlot {
      std::string name;
      std::string description;
      bool required = false;
      ITexture::Format preferredFormat = ITexture::Format::RGBA8;
   };

   MaterialTemplate(const std::string_view typeName);
   // Property definitions
   MaterialTemplate& AddProperty(const std::string& name, const IMaterial::PropertyValue& defaultValue,
                                 const std::string& description = "");
   // Texture slot definitions
   MaterialTemplate& AddTextureSlot(const std::string& name, const std::string& description = "",
                                    const bool required = false, const ITexture::Format preferredFormat = ITexture::Format::RGBA8);

   const std::string& GetTypeName() const noexcept;
   const std::unordered_map<std::string, IMaterial::PropertyInfo>& GetProperties() const noexcept;
   const std::unordered_map<std::string, TextureSlot>& GetTextureSlots() const noexcept;

private:
   std::string m_typeName;
   std::unordered_map<std::string, IMaterial::PropertyInfo> m_properties;
   std::unordered_map<std::string, TextureSlot> m_textureSlots;
};

