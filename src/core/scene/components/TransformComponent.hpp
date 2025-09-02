#pragma once

#include "core/Transform.hpp"

#include "core/scene/components/Component.hpp"

class TransformComponent final : public Component {
  public:
   TransformComponent() = default;
   explicit TransformComponent(const Transform& transform) : m_transform(transform) {}

   void DrawInspector(Node* node) override;

   [[nodiscard]] const glm::vec3& GetPosition() const noexcept;
   void SetPosition(const glm::vec3& newPos);
   [[nodiscard]] const glm::vec3& GetRotation() const noexcept;
   void SetRotation(const glm::vec3& newRotation);
   [[nodiscard]] const glm::vec3& GetScale() const noexcept;
   void SetScale(const glm::vec3& newScale);

   [[nodiscard]] const Transform& GetTransform() const noexcept;
   [[nodiscard]] Transform& GetMutableTransform() noexcept;

  private:
   Transform m_transform;
};
