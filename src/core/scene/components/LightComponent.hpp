#pragma once

#include <glm/glm.hpp>

#include "core/scene/components/Component.hpp"

class LightComponent : public Component {
public:
   enum class LightType {
      Directional,
      Point,
      Spot
   };

   LightComponent();
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

