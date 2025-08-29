#include "core/resource/MaterialTemplate.hpp"

MaterialTemplate::MaterialTemplate(const std::string_view typeName)
   :
   m_typeName(std::move(typeName))
{}

MaterialTemplate& MaterialTemplate::AddProperty(const std::string& name, const IMaterial::PropertyValue& defaultValue,
                                                const std::string& description) {
   IMaterial::PropertyInfo info;
   info.name = name;
   info.defaultValue = defaultValue;
   info.description = description;
   // Determine type from variant
   std::visit([&info](auto&& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, float>) info.type = IMaterial::PropertyType::Float;
      else if constexpr (std::is_same_v<T, glm::vec2>) info.type = IMaterial::PropertyType::Vec2;
      else if constexpr (std::is_same_v<T, glm::vec3>) info.type = IMaterial::PropertyType::Vec3;
      else if constexpr (std::is_same_v<T, glm::vec4>) info.type = IMaterial::PropertyType::Vec4;
      else if constexpr (std::is_same_v<T, int32_t>) info.type = IMaterial::PropertyType::Int;
      else if constexpr (std::is_same_v<T, uint32_t>) info.type = IMaterial::PropertyType::UInt;
      else if constexpr (std::is_same_v<T, bool>) info.type = IMaterial::PropertyType::Bool;
      else if constexpr (std::is_same_v<T, TextureHandle>) info.type = IMaterial::PropertyType::Texture;
   }, defaultValue);
   m_properties[name] = std::move(info);
   return *this;
}

MaterialTemplate& MaterialTemplate::AddTextureSlot(const std::string& name, const std::string& description,
                                                   const bool required, const ITexture::Format preferredFormat) {
   TextureSlot slot;
   slot.name = name;
   slot.description = description;
   slot.required = required;
   slot.preferredFormat = preferredFormat;
   m_textureSlots[name] = std::move(slot);
   return *this;
}

const std::string& MaterialTemplate::GetTypeName() const noexcept {
   return m_typeName;
}

const std::unordered_map<std::string, IMaterial::PropertyInfo>& MaterialTemplate::GetProperties() const noexcept {
   return m_properties;
}

const std::unordered_map<std::string, MaterialTemplate::TextureSlot>& MaterialTemplate::GetTextureSlots() const noexcept {
   return m_textureSlots;
}

