#include "core/resource/MaterialInstance.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <stdexcept>
#include <span>

MaterialInstance::MaterialInstance(const MaterialTemplate& materialTemplate)
    : m_template(&materialTemplate) {
   if (!m_template->IsFinalized()) {
      throw std::runtime_error("Material template must be finalized before creating instances");
   }
   // Initialize parameters with defaults
   for (const auto& [name, desc] : m_template->GetParameters()) {
      m_parameters.emplace(name, desc.defaultValue);
   }
   // Initialize textures with defaults
   for (const auto& [name, desc] : m_template->GetTextures()) {
      m_textures.emplace(name, desc.defaultTexture);
   }
   // Allocate UBO data
   m_uboData.resize(m_template->GetUBOSize(), std::byte{0});
}

size_t MaterialInstance::GetMemoryUsage() const noexcept {
   return m_uboData.size() + (m_parameters.size() * 32) + (m_textures.size() * 16);
}

void MaterialInstance::SetParameter(const std::string_view name, const MaterialParam& value) {
   if (auto it = m_parameters.find(std::string(name)); it != m_parameters.end()) {
      it->second = value;
      WriteParamToUBO(name, value);
      m_uboDirty = true;
   }
}

MaterialParam MaterialInstance::GetParameter(const std::string_view name) const {
   if (auto it = m_parameters.find(std::string(name)); it != m_parameters.end()) {
      return it->second;
   }
   return {};
}

bool MaterialInstance::HasParameter(const std::string_view name) const noexcept {
   return m_parameters.contains(std::string(name));
}

void MaterialInstance::SetTexture(const std::string_view name, const TextureHandle texture) {
   if (auto it = m_textures.find(std::string(name)); it != m_textures.end()) {
      it->second = texture;
   }
}

TextureHandle MaterialInstance::GetTexture(const std::string_view name) const {
   if (auto it = m_textures.find(std::string(name)); it != m_textures.end()) {
      return it->second;
   }
   return {};
}

bool MaterialInstance::HasTexture(const std::string_view name) const noexcept {
   return m_textures.contains(std::string(name));
}

std::string_view MaterialInstance::GetTemplateName() const noexcept {
   return m_template->GetName();
}

uint32_t MaterialInstance::GetUBOSize() const noexcept { return m_template->GetUBOSize(); }

void MaterialInstance::UpdateUBOData() {
   if (!m_uboDirty) [[likely]]
      return;
   for (const auto& [name, value] : m_parameters) {
      WriteParamToUBO(name, value);
   }
   m_uboDirty = false;
}

void MaterialInstance::WriteParamToUBO(const std::string_view name, const MaterialParam& value) {
   const auto& params = m_template->GetParameters();
   if (auto paramIt = params.find(std::string(name)); paramIt != params.end()) {
      const auto& desc = paramIt->second;
      const auto target = std::span{m_uboData}.subspan(desc.offset);
      std::visit(
         [&target](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int32_t> ||
                          std::is_same_v<T, uint32_t>) {
               if (target.size() >= sizeof(T)) {
                  std::memcpy(target.data(), &val, sizeof(T));
               }
            } else if constexpr (std::is_same_v<T, glm::vec2>) {
               if (target.size() >= sizeof(float) * 2) {
                  std::memcpy(target.data(), glm::value_ptr(val), sizeof(float) * 2);
               }
            } else if constexpr (std::is_same_v<T, glm::vec3>) {
               if (target.size() >= 16) {
                  const glm::vec4 padded(val, 0.0f);
                  std::memcpy(target.data(), glm::value_ptr(padded), 16);
               }
            } else if constexpr (std::is_same_v<T, glm::vec4>) {
               if (target.size() >= 16) {
                  std::memcpy(target.data(), glm::value_ptr(val), 16);
               }
            } else if constexpr (std::is_same_v<T, glm::mat2>) {
               if (target.size() >= 32) {
                  const glm::vec4 col0(val[0][0], val[0][1], 0.0f, 0.0f);
                  const glm::vec4 col1(val[1][0], val[1][1], 0.0f, 0.0f);
                  std::memcpy(target.data(), glm::value_ptr(col0), 16);
                  std::memcpy(target.data() + 16, glm::value_ptr(col1), 16);
               }
            } else if constexpr (std::is_same_v<T, glm::mat3>) {
               if (target.size() >= 48) {
                  const glm::vec4 col0(val[0][0], val[0][1], val[0][2], 0.0f);
                  const glm::vec4 col1(val[1][0], val[1][1], val[1][2], 0.0f);
                  const glm::vec4 col2(val[2][0], val[2][1], val[2][2], 0.0f);
                  std::memcpy(target.data(), glm::value_ptr(col0), 16);
                  std::memcpy(target.data() + 16, glm::value_ptr(col1), 16);
                  std::memcpy(target.data() + 32, glm::value_ptr(col2), 16);
               }
            } else if constexpr (std::is_same_v<T, glm::mat4>) {
               if (target.size() >= 64) {
                  std::memcpy(target.data(), glm::value_ptr(val), 64);
               }
            }
         },
         value);
   }
}
