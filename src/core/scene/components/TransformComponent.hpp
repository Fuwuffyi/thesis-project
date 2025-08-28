#pragma once

#include "core/Transform.hpp"

#include "core/scene/components/Component.hpp"

class TransformComponent : public Component {
public:
   const glm::vec3& GetPosition() const;
   void SetPosition(const glm::vec3& newPos);
   const glm::vec3& GetRotation() const;
   void SetRotation(const glm::vec3& newRotation);
   const glm::vec3& GetScale() const;
   void SetScale(const glm::vec3& newScale);

   Transform* GetTransform();

private:
   Transform m_transform;
};

