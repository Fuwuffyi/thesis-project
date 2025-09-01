#pragma once

#include "core/scene/components/Component.hpp"

#include <glm/glm.hpp>

class LightComponent final : public Component {
public:
   enum class LightType : uint32_t {
      Directional = 0,
      Point = 1,
      Spot = 2
   };

   LightComponent();

   void DrawInspector(Node* node) override;

   void SetType(const LightType type) noexcept;
   [[nodiscard]] LightType GetType() const noexcept;

   void SetColor(const glm::vec3& color) noexcept;
   [[nodiscard]] const glm::vec3& GetColor() const noexcept;

   void SetIntensity(const float intensity) noexcept;
   [[nodiscard]] float GetIntensity() const noexcept;

   void SetConstant(const float constant) noexcept;
   [[nodiscard]] float GetConstant() const noexcept;

   void SetLinear(const float linear) noexcept;
   [[nodiscard]] float GetLinear() const noexcept;

   void SetQuadratic(const float quadratic) noexcept;
   [[nodiscard]] float GetQuadratic() const noexcept;

   void SetInnerCone(const float innerCone) noexcept;
   [[nodiscard]] float GetInnerCone() const noexcept;

   void SetOuterCone(const float outerCone) noexcept;
   [[nodiscard]] float GetOuterCone() const noexcept;

private:
   // Generic data
   LightType m_type;
   glm::vec3 m_color;

   // Light strength
   float m_intensity;
   float m_constant;
   float m_linear;
   float m_quadratic;

   // Spotlight specifics
   float m_innerCone;
   float m_outerCone;

   // Shadow mapping
   bool m_castsShadows;
};

