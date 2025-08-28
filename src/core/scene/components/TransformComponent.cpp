#include "core/scene/components/TransformComponent.hpp"

[[nodiscard]] const glm::vec3& TransformComponent::GetPosition() const noexcept {
   return m_transform.GetPosition();
}

void TransformComponent::SetPosition(const glm::vec3& newPos) {
   m_transform.SetPosition(newPos);
}

[[nodiscard]] const glm::vec3& TransformComponent::GetRotation() const noexcept {
   return m_transform.GetEulerAngles();
}

void TransformComponent::SetRotation(const glm::vec3& newRotation) {
   m_transform.SetRotation(newRotation);
}

[[nodiscard]] const glm::vec3& TransformComponent::GetScale() const noexcept {
   return m_transform.GetScale();
}

void TransformComponent::SetScale(const glm::vec3& newScale) {
   m_transform.SetScale(newScale);
}

[[nodiscard]] const Transform& TransformComponent::GetTransform() const noexcept {
   return m_transform;
}

[[nodiscard]] Transform& TransformComponent::GetMutableTransform() noexcept {
   return m_transform;
}
