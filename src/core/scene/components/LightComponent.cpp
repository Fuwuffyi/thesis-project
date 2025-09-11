#include "core/scene/components/LightComponent.hpp"

#include <imgui.h>

LightComponent::LightComponent()
    : m_type(LightType::Point),
      m_color(1.0f),
      m_intensity(1.0f),
      m_innerCone(glm::radians(30.0f)),
      m_outerCone(glm::radians(45.0f)),
      m_constant(1.0f),
      m_linear(0.09f),
      m_quadratic(0.032f),
      m_castsShadows(false) {}

void LightComponent::DrawInspector(Node* const node) {
   if (ImGui::CollapsingHeader(
          "Light", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      // Light type selection
      constexpr const char* lightTypes[] = {"Directional", "Point", "Spot"};
      int32_t currentType = static_cast<int32_t>(m_type);
      if (ImGui::Combo("Light Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes))) {
         SetType(static_cast<LightType>(currentType));
      }
      // Color and intensity
      ImGui::ColorEdit3("Color", &m_color.x);
      ImGui::SliderFloat("Intensity", &m_intensity, 0.0f, 10.0f);
      // Attenuation for point/spot lights
      if (m_type != LightType::Directional) {
         ImGui::SliderFloat("Constant", &m_constant, 0.0f, 2.0f);
         ImGui::SliderFloat("Linear", &m_linear, 0.0f, 1.0f);
         ImGui::SliderFloat("Quadratic", &m_quadratic, 0.0f, 1.0f);
      }
      // Spotlight specific
      if (m_type == LightType::Spot) {
         ImGui::SliderAngle("Inner Cone", &m_innerCone, 0.0f, 90.0f);
         ImGui::SliderAngle("Outer Cone", &m_outerCone, 0.0f, 90.0f);
      }
      ImGui::Checkbox("Casts Shadows", &m_castsShadows);
   }
}

void LightComponent::SetType(const LightType type) noexcept { m_type = type; }

void LightComponent::SetColor(const glm::vec3& color) noexcept { m_color = color; }

void LightComponent::SetIntensity(const float intensity) noexcept { m_intensity = intensity; }

void LightComponent::SetConstant(const float constant) noexcept { m_constant = constant; }

void LightComponent::SetLinear(const float linear) noexcept { m_linear = linear; }

void LightComponent::SetQuadratic(const float quadratic) noexcept { m_quadratic = quadratic; }

void LightComponent::SetInnerCone(const float innerCone) noexcept { m_innerCone = innerCone; }

void LightComponent::SetOuterCone(const float outerCone) noexcept { m_outerCone = outerCone; }

void LightComponent::SetCastsShadows(const bool castsShadows) noexcept {
   m_castsShadows = castsShadows;
}
