#pragma once

#include "core/scene/components/Component.hpp"

#include "core/resource/ResourceHandle.hpp"

class RendererComponent final : public Component {
public:
   RendererComponent() = default;
   explicit RendererComponent(MeshHandle mesh);

   // Mesh management
   void SetMesh(MeshHandle mesh);
   [[nodiscard]] const MeshHandle& GetMesh() const noexcept;
   [[nodiscard]] bool HasMesh() const noexcept;

   // TODO: Add material setup

   // Rendering properties
   void SetVisible(const bool visible) noexcept;
   [[nodiscard]] bool IsVisible() const noexcept;

   void SetCastsShadows(const bool castsShadows) noexcept;
   [[nodiscard]] bool CastsShadows() const noexcept;

   void SetReceivesShadows(const bool receivesShadows) noexcept;
   [[nodiscard]] bool ReceivesShadows() const noexcept;

private:
   MeshHandle m_mesh;

   bool m_visible;
   bool m_castsShadows;
   bool m_receivesShadows;
};

