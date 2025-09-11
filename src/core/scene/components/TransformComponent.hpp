#pragma once

#include "core/Transform.hpp"
#include "core/scene/components/Component.hpp"

class TransformComponent final : public Component {
  public:
   TransformComponent() = default;
   explicit TransformComponent(const Transform& transform) : m_transform(transform) {}

   void DrawInspector(Node* const node) override;

   [[nodiscard]] constexpr const glm::vec3& GetPosition() const noexcept {
      return m_transform.GetPosition();
   }
   void SetPosition(const glm::vec3& newPos);

   [[nodiscard]] constexpr const glm::vec3& GetRotation() const noexcept {
      return m_transform.GetEulerAngles();
   }
   void SetRotation(const glm::vec3& newRotation);

   [[nodiscard]] constexpr const glm::vec3& GetScale() const noexcept {
      return m_transform.GetScale();
   }
   void SetScale(const glm::vec3& newScale);

   [[nodiscard]] constexpr const Transform& GetTransform() const noexcept { return m_transform; }
   [[nodiscard]] constexpr Transform& GetMutableTransform() noexcept { return m_transform; }

  private:
   Transform m_transform;
};
