#include "core/scene/components/LightComponent.hpp"

#include <imgui.h>

LightComponent::LightComponent()
   :
   m_type(LightType::Point),
   m_color(glm::vec3(1.0)),
   m_innerCone(0.8),
   m_outerCone(0.9),
   m_attenuation(1.0),
   m_castsShadows(false)
{}

void LightComponent::DrawInspector(Node* node) {
   if (ImGui::CollapsingHeader("Light",
                               ImGuiTreeNodeFlags_DefaultOpen |
                               ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      const char* lightTypes[] = { "Point", "Directional", "Spot" };
      int currentType = static_cast<int>(m_type);
      if (ImGui::Combo("Light Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes))) {
         SetType(static_cast<LightType>(currentType));
      }
      ImGui::ColorEdit3("Color", &m_color.x);
      ImGui::SliderFloat("Attenuation", &m_attenuation, 0.0f, 1.0f);
      if (m_type == LightType::Spot) {
         ImGui::SliderFloat("Inner Cone", &m_innerCone, 0.0f, 1.0f);
         ImGui::SliderFloat("Outer Cone", &m_outerCone, 0.0f, 1.0f);
      }
      ImGui::Checkbox("Casts Shadows", &m_castsShadows);
   }
}

void LightComponent::SetType(const LightType type) noexcept {
   m_type = type;
}

[[nodiscard]] LightComponent::LightType LightComponent::GetType() const noexcept {
   return m_type;
}

void LightComponent::SetColor(const glm::vec3& color) noexcept {
   m_color = color;
}

[[nodiscard]] const glm::vec3& LightComponent::GetColor() const noexcept {
   return m_color;
}

void LightComponent::SetInnerCone(const float innerCone) noexcept {
   m_innerCone = innerCone;
}

[[nodiscard]] float LightComponent::GetInnerCone() const noexcept {
   return m_innerCone;
}

void LightComponent::SetOuterCone(const float outerCone) noexcept {
   m_outerCone = outerCone;
}

[[nodiscard]] float LightComponent::GetOuterCone() const noexcept {
   return m_outerCone;
}

void LightComponent::SetAttenuation(const float attenuation) noexcept {
   m_attenuation = attenuation;
}

[[nodiscard]] float LightComponent::GetAttenuation() const noexcept {
   return m_attenuation;
}

