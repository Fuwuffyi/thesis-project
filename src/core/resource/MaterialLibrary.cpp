#include "core/resource/MaterialLibrary.hpp"

#include "core/resource/MaterialTemplate.hpp"

void MaterialLibrary::RegisterTemplate(std::unique_ptr<MaterialTemplate> materialTemplate) {
   if (materialTemplate) {
      const std::string typeName = materialTemplate->GetTypeName();
      m_templates[typeName] = std::move(materialTemplate);
   }
}

const MaterialTemplate* MaterialLibrary::GetTemplate(const std::string& typeName) const {
   if (auto it = m_templates.find(typeName); it != m_templates.end()) {
      return it->second.get();
   }
   return nullptr;
}

bool MaterialLibrary::HasTemplate(const std::string& typeName) const {
   return m_templates.find(typeName) != m_templates.end();
}

std::vector<std::string> MaterialLibrary::GetTemplateNames() const {
   std::vector<std::string> names;
   for (const auto& [name, _] : m_templates) {
      names.push_back(name);
   }
   return names;
}

void MaterialLibrary::RegisterStandardTemplates(MaterialLibrary& library) {
   // PBR Material
   std::unique_ptr<MaterialTemplate> pbrTemplate = std::make_unique<MaterialTemplate>("PBR");
   pbrTemplate->AddProperty("albedo", glm::vec3(1.0f, 1.0f, 1.0f), "Base color")
      .AddProperty("roughness", 0.5f, "Surface roughness")
      .AddProperty("metallic", 0.0f, "Metallic factor")
      .AddProperty("emissive", glm::vec3(0.0f), "Emissive color")
      .AddTextureSlot("albedoMap", "Albedo texture", false)
      .AddTextureSlot("roughnessMap", "Roughness texture", false)
      .AddTextureSlot("metallicMap", "Metallic texture", false)
      .AddTextureSlot("normalMap", "Normal map", false)
      .AddTextureSlot("emissiveMap", "Emissive texture", false)
      .AddTextureSlot("displacementMap", "Displacement texture", false)
      .AddTextureSlot("aoMap", "Ambient occlusion map", false);
   library.RegisterTemplate(std::move(pbrTemplate));
}

