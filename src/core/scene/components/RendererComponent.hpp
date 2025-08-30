#pragma once

#include "core/scene/components/Component.hpp"

#include "core/resource/IMesh.hpp"
#include "core/resource/IMaterial.hpp"

#include <vector>

class RendererComponent final : public Component {
public:
   struct SubMeshRenderer {
      MeshHandle mesh;
      MaterialHandle material;
      bool visible = true;
      bool castsShadows = true;
      bool receivesShadows = true;
   };

   RendererComponent() = default;
   explicit RendererComponent(const MeshHandle mesh, const MaterialHandle material = MaterialHandle{});
   explicit RendererComponent(const std::vector<MeshHandle>& meshes, const std::vector<MaterialHandle>& materials);

   void DrawInspector(Node* node) override;

   // Mesh management
   void SetMesh(const MeshHandle mesh);
   void SetMaterial(const MaterialHandle material);
   [[nodiscard]] const MeshHandle& GetMesh() const noexcept;
   [[nodiscard]] const MaterialHandle& GetMaterial() const noexcept;
   [[nodiscard]] bool HasMesh() const noexcept;

   // Submesh management
   void SetSubMeshRenderers(const std::vector<SubMeshRenderer>& renderers);
   void AddSubMeshRenderer(const SubMeshRenderer& renderer);
   void RemoveSubMeshRenderer(const size_t index);
   [[nodiscard]] const std::vector<SubMeshRenderer>& GetSubMeshRenderers() const noexcept;
   [[nodiscard]] std::vector<SubMeshRenderer>& GetSubMeshRenderers() noexcept;
   [[nodiscard]] size_t GetSubMeshCount() const noexcept;

   // Individual sub-mesh control
   void SetSubMeshVisible(const size_t index, const bool visible);
   void SetSubMeshMaterial(const size_t index, const MaterialHandle material);
   [[nodiscard]] bool IsSubMeshVisible(const size_t index) const;

   // Rendering properties
   void SetVisible(const bool visible) noexcept;
   [[nodiscard]] bool IsVisible() const noexcept;

   void SetCastsShadows(const bool castsShadows) noexcept;
   [[nodiscard]] bool CastsShadows() const noexcept;

   void SetReceivesShadows(const bool receivesShadows) noexcept;
   [[nodiscard]] bool ReceivesShadows() const noexcept;

   // Utility methods
   [[nodiscard]] bool IsMultiMesh() const noexcept;
   void ClearSubMeshes();

private:
   MeshHandle m_mesh;
   MaterialHandle m_material;
   std::vector<SubMeshRenderer> m_subMeshRenderers;

   bool m_visible;
   bool m_castsShadows;
   bool m_receivesShadows;
};

