#include "core/scene/components/TransformComponent.hpp"

#include "core/scene/Node.hpp"

#include <imgui.h>

void TransformComponent::DrawInspector(Node* const node) {
   if (ImGui::CollapsingHeader(
          "Transform", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      glm::vec3 posInput = this->m_transform.GetPosition();
      if (ImGui::DragFloat3("Position", &posInput.x, 0.01f)) {
         this->m_transform.SetPosition(posInput);
         node->MarkTransformDirty();
      }
      glm::vec3 eulerAngles = glm::degrees(this->m_transform.GetEulerAngles());
      if (ImGui::DragFloat3("Rotation", &eulerAngles.x, 0.1f, -180.0f, 180.0f)) {
         this->m_transform.SetRotation(glm::radians(eulerAngles));
         node->MarkTransformDirty();
      }
      glm::vec3 scaleInput = this->m_transform.GetScale();
      if (ImGui::DragFloat3("Scale", &scaleInput.x, 0.01f, 0.01f, 100.0f)) {
         this->m_transform.SetScale(scaleInput);
         node->MarkTransformDirty();
      }
   }
}

void TransformComponent::SetPosition(const glm::vec3& newPos) { m_transform.SetPosition(newPos); }

void TransformComponent::SetRotation(const glm::vec3& newRotation) {
   m_transform.SetRotation(newRotation);
}

void TransformComponent::SetScale(const glm::vec3& newScale) { m_transform.SetScale(newScale); }
