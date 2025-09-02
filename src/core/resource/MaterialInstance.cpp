#include "core/resource/MaterialInstance.hpp"

#include <cstring>
#include <stdexcept>

MaterialInstance::MaterialInstance(const MaterialTemplate& materialTemplate)
    : m_template(&materialTemplate) {
   if (!m_template->IsFinalized()) {
      throw std::runtime_error("Material template must be finalized before creating instances");
   }
   // Initialize parameters with defaults
   for (const auto& [name, desc] : m_template->GetParameters()) {
      m_parameters[name] = desc.defaultValue;
   }
   // Initialize textures with defaults
   for (const auto& [name, desc] : m_template->GetTextures()) {
      m_textures[name] = desc.defaultTexture;
   }
   // Allocate UBO data
   m_uboData.resize(m_template->GetUBOSize());
   std::memset(m_uboData.data(), 0, m_uboData.size());
}

size_t MaterialInstance::GetMemoryUsage() const {
   return m_uboData.size() + (m_parameters.size() * 32) + (m_textures.size() * 16);
}

void MaterialInstance::SetParameter(const std::string& name, const MaterialParam& value) {
   auto it = m_parameters.find(name);
   if (it != m_parameters.end()) {
      it->second = value;
      WriteParamToUBO(name, value);
      m_uboDirty = true;
   }
}

MaterialParam MaterialInstance::GetParameter(const std::string& name) const {
   auto it = m_parameters.find(name);
   return it != m_parameters.end() ? it->second : MaterialParam{};
}

bool MaterialInstance::HasParameter(const std::string& name) const {
   return m_parameters.find(name) != m_parameters.end();
}

void MaterialInstance::SetTexture(const std::string& name, const TextureHandle& texture) {
   auto it = m_textures.find(name);
   if (it != m_textures.end()) {
      it->second = texture;
   }
}

TextureHandle MaterialInstance::GetTexture(const std::string& name) const {
   auto it = m_textures.find(name);
   return it != m_textures.end() ? it->second : TextureHandle{};
}

bool MaterialInstance::HasTexture(const std::string& name) const {
   return m_textures.find(name) != m_textures.end();
}

const std::string& MaterialInstance::GetTemplateName() const { return m_template->GetName(); }

uint32_t MaterialInstance::GetUBOSize() const { return m_template->GetUBOSize(); }

void MaterialInstance::UpdateUBOData() {
   if (!m_uboDirty)
      return;

   for (const auto& [name, value] : m_parameters) {
      WriteParamToUBO(name, value);
   }

   m_uboDirty = false;
}

void MaterialInstance::WriteParamToUBO(const std::string& name, const MaterialParam& value) {
   auto paramIt = m_template->GetParameters().find(name);
   if (paramIt == m_template->GetParameters().end()) {
      return;
   }

   const auto& desc = paramIt->second;
   uint8_t* target = m_uboData.data() + desc.offset;

   std::visit(
      [target](const auto& val) {
         using T = std::decay_t<decltype(val)>;
         if constexpr (std::is_same_v<T, glm::mat3>) {
            // mat3 in std140 is stored as 3 vec4s
            glm::vec4 col0(val[0], 0.0f);
            glm::vec4 col1(val[1], 0.0f);
            glm::vec4 col2(val[2], 0.0f);
            std::memcpy(target, &col0, 16);
            std::memcpy(target + 16, &col1, 16);
            std::memcpy(target + 32, &col2, 16);
         } else if constexpr (std::is_same_v<T, glm::vec3>) {
            // vec3 alignment in std140
            std::memcpy(target, &val, 12);
         } else {
            std::memcpy(target, &val, sizeof(val));
         }
      },
      value);
}
