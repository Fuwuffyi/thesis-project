#pragma once

#include "Component.hpp"
#include "../../resource/ResourceHandle.hpp"

class RendererComponent : public Component {
public:
   RendererComponent() = default;
   explicit RendererComponent(const MeshHandle& mesh);

   ~RendererComponent() override = default;

   // Mesh management
   void SetMesh(const MeshHandle& mesh);
   const MeshHandle& GetMesh() const;
   bool HasMesh() const;

   // TODO: Add material setup

   // Rendering properties
   void SetVisible(const bool visible);
   bool IsVisible() const;

   void SetCastsShadows(const bool castsShadows);
   bool CastsShadows() const;

   void SetReceivesShadows(const bool receivesShadows);
   bool ReceivesShadows() const;

private:
   MeshHandle m_mesh;

   bool m_visible;
   bool m_castsShadows;
   bool m_receivesShadows;
};

