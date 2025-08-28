#include "core/scene/components/TransformComponent.hpp"

const glm::vec3& TransformComponent::GetPosition() const {
   return m_transform.GetPosition();
}

void TransformComponent::SetPosition(const glm::vec3& newPos) {
   m_transform.SetPosition(newPos);
}

const glm::vec3& TransformComponent::GetRotation() const {
   return m_transform.GetEulerAngles();
}

void TransformComponent::SetRotation(const glm::vec3& newRotation) {
   m_transform.SetRotation(newRotation);
}

const glm::vec3& TransformComponent::GetScale() const {
   return m_transform.GetScale();
}

void TransformComponent::SetScale(const glm::vec3& newScale) {
   m_transform.SetScale(newScale);
}

Transform* TransformComponent::GetTransform() {
   return &m_transform;
}

