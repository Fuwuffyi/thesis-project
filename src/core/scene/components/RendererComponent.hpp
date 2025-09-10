#pragma once

#include "core/scene/components/Component.hpp"

#include "core/resource/IMesh.hpp"
#include "core/resource/IMaterial.hpp"

class RendererComponent final : public Component {
  public:
   RendererComponent() = default;
   explicit RendererComponent(const MeshHandle mesh, const MaterialHandle material);

   void DrawInspector(Node* node) override;

   // Mesh management
   void SetMesh(const MeshHandle mesh);
   [[nodiscard]] const MeshHandle& GetMesh() const noexcept;
   [[nodiscard]] bool HasMesh() const noexcept;

   // Material management
   void SetMaterial(const MaterialHandle material);
   [[nodiscard]] const MaterialHandle& GetMaterial() const noexcept;
   [[nodiscard]] bool HasMaterial() const noexcept;

   // Rendering properties
   void SetVisible(const bool visible) noexcept;
   [[nodiscard]] bool IsVisible() const noexcept;

   void SetCastsShadows(const bool castsShadows) noexcept;
   [[nodiscard]] bool CastsShadows() const noexcept;

   void SetReceivesShadows(const bool receivesShadows) noexcept;
   [[nodiscard]] bool ReceivesShadows() const noexcept;

  private:
   MeshHandle m_mesh;
   MaterialHandle m_material;

   bool m_visible;
   bool m_castsShadows;
   bool m_receivesShadows;
};
