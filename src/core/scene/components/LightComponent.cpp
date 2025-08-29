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
   }
}

