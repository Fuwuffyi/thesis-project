#include "core/resource/Material.hpp"
#include <stdexcept>

Material::Material(const MaterialTemplate& materialTemplate)
   :
   m_template(materialTemplate),
   m_typeName(materialTemplate.GetTypeName())\
{
   for (const auto& [name, info] : m_template.GetProperties()) {
      m_properties[name] = info.defaultValue;
   }
}

Material::Material(const std::string& typeName, const MaterialTemplate& materialTemplate)
   :
   m_template(materialTemplate),
   m_typeName(typeName)
{
   for (const auto& [name, info] : m_template.GetProperties()) {
      m_properties[name] = info.defaultValue;
   }
}

ResourceType Material::GetType() const {
   return ResourceType::Material;
}

size_t Material::GetMemoryUsage() const {
   size_t size = sizeof(*this);
   size += m_typeName.size();
   size += m_properties.size() * (sizeof(std::string) + sizeof(PropertyValue));
   size += m_textures.size() * (sizeof(std::string) + sizeof(TextureHandle));
   return size;
}

bool Material::IsValid() const {
   return true;
}

void Material::SetProperty(const std::string& name, const PropertyValue& value) {
   if (m_template.GetProperties().find(name) == m_template.GetProperties().end()) {
      throw std::runtime_error("Property" + name + " not in template");
   }
   m_properties[name] = value;
}

IMaterial::PropertyValue Material::GetProperty(const std::string& name) const {
   if (auto it = m_properties.find(name); it != m_properties.end()) {
      return it->second;
   }
   // Return default from template if exists
   const auto& templateProps = m_template.GetProperties();
   if (auto templateIt = templateProps.find(name); templateIt != templateProps.end()) {
      return templateIt->second.defaultValue;
   }
   // Return empty variant if not found
   return PropertyValue{};
}

bool Material::HasProperty(const std::string& name) const {
   return m_properties.find(name) != m_properties.end() ||
   m_template.GetProperties().find(name) != m_template.GetProperties().end();
}

void Material::RemoveProperty(const std::string& name) {
   m_properties.erase(name);
}

void Material::SetTexture(const std::string& slotName, const TextureHandle texture) {
   // Validate slot exists in template
   if (m_template.GetTextureSlots().find(slotName) == m_template.GetTextureSlots().end()) {
      // Could throw or log warning - slot not in template
      return;
   }
   m_textures[slotName] = texture;
}

TextureHandle Material::GetTexture(const std::string& slotName) const {
   if (auto it = m_textures.find(slotName); it != m_textures.end()) {
      return it->second;
   }
   return TextureHandle{};
}

bool Material::HasTexture(const std::string& slotName) const {
   return m_textures.find(slotName) != m_textures.end() && m_textures.at(slotName).IsValid();
}

void Material::RemoveTexture(const std::string& slotName) {
   m_textures.erase(slotName);
}

std::vector<std::string> Material::GetPropertyNames() const {
   std::vector<std::string> names;
   for (const auto& [name, _] : m_template.GetProperties()) {
      names.push_back(name);
   }
   return names;
}

std::vector<std::string> Material::GetTextureSlotNames() const {
   std::vector<std::string> names;
   for (const auto& [name, _] : m_template.GetTextureSlots()) {
      names.push_back(name);
   }
   return names;
}

const std::string& Material::GetMaterialType() const {
   return m_typeName;
}

const MaterialTemplate& Material::GetTemplate() const {
   return m_template;
}
