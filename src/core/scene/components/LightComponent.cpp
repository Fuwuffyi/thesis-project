#include "core/scene/components/LightComponent.hpp"

LightComponent::LightComponent()
   :
   m_type(LightType::Point),
   m_color(glm::vec3(1.0)),
   m_innerCone(0.8),
   m_outerCone(0.9),
   m_attenuation(1.0),
   m_castsShadows(false)
{}

