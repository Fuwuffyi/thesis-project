#pragma once

#include "core/scene/components/Component.hpp"

#include <glm/glm.hpp>

class LightComponent final : public Component {
  public:
   enum class LightType : uint32_t { Directional = 0, Point = 1, Spot = 2 };

   LightComponent();

   void DrawInspector(Node* const node) override;

   void SetType(const LightType type) noexcept;
   [[nodiscard]] constexpr LightType GetType() const noexcept { return m_type; }

   void SetColor(const glm::vec3& color) noexcept;
   [[nodiscard]] constexpr const glm::vec3& GetColor() const noexcept { return m_color; }

   void SetIntensity(const float intensity) noexcept;
   [[nodiscard]] constexpr float GetIntensity() const noexcept { return m_intensity; }

   void SetConstant(const float constant) noexcept;
   [[nodiscard]] constexpr float GetConstant() const noexcept { return m_constant; }

   void SetLinear(const float linear) noexcept;
   [[nodiscard]] constexpr float GetLinear() const noexcept { return m_linear; }

   void SetQuadratic(const float quadratic) noexcept;
   [[nodiscard]] constexpr float GetQuadratic() const noexcept { return m_quadratic; }

   void SetInnerCone(const float innerCone) noexcept;
   [[nodiscard]] constexpr float GetInnerCone() const noexcept { return m_innerCone; }

   void SetOuterCone(const float outerCone) noexcept;
   [[nodiscard]] constexpr float GetOuterCone() const noexcept { return m_outerCone; }

   void SetCastsShadows(const bool castsShadows) noexcept;
   [[nodiscard]] constexpr bool GetCastsShadows() const noexcept { return m_castsShadows; }

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
