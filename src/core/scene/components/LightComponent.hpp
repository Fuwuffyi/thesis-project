#pragma once

#include "core/scene/components/Component.hpp"

#include <glm/glm.hpp>

class LightComponent final : public Component {
public:
   enum class LightType {
      Directional,
      Point,
      Spot
   };

   LightComponent();

   void DrawInspector(Node* node) override;

private:
   // Generic data
   LightType m_type;
   glm::vec3 m_color;

   // Spotlight specifics
   float m_innerCone;
   float m_outerCone;

   // Light strength
   float m_attenuation;
   bool m_castsShadows;
};

