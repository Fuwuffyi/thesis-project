#pragma once

#include "core/scene/components/Component.hpp"
#include "core/resource/IMesh.hpp"
#include "core/resource/IMaterial.hpp"

class RendererComponent final : public Component {
  public:
   RendererComponent() = default;
   explicit RendererComponent(const MeshHandle mesh, const MaterialHandle material);

   void DrawInspector(Node* const node) override;

   // Mesh management
   void SetMesh(const MeshHandle mesh);
   [[nodiscard]] constexpr const MeshHandle& GetMesh() const noexcept { return m_mesh; }
   [[nodiscard]] bool HasMesh() const noexcept;

   // Material management
   void SetMaterial(const MaterialHandle material);
   [[nodiscard]] constexpr const MaterialHandle& GetMaterial() const noexcept { return m_material; }
   [[nodiscard]] bool HasMaterial() const noexcept;

   // Rendering properties
   void SetVisible(const bool visible) noexcept;
   [[nodiscard]] constexpr bool IsVisible() const noexcept { return m_visible; }

   void SetCastsShadows(const bool castsShadows) noexcept;
   [[nodiscard]] constexpr bool CastsShadows() const noexcept { return m_castsShadows; }

   void SetReceivesShadows(const bool receivesShadows) noexcept;
   [[nodiscard]] constexpr bool ReceivesShadows() const noexcept { return m_receivesShadows; }

  private:
   MeshHandle m_mesh;
   MaterialHandle m_material;

   bool m_visible{true};
   bool m_castsShadows{true};
   bool m_receivesShadows{true};
};
