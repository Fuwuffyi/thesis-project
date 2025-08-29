#pragma once

#include "core/scene/components/Component.hpp"
#include "core/resource/ResourceHandle.hpp"

#include <vector>

class RendererComponent final : public Component {
public:
   struct SubMeshRenderer {
      MeshHandle mesh;
      bool visible = true;
      bool castsShadows = true;
      bool receivesShadows = true;
      uint32_t materialIndex = 0;
   };

   RendererComponent() = default;
   explicit RendererComponent(MeshHandle mesh);
   explicit RendererComponent(const std::vector<MeshHandle>& meshes, const std::vector<uint32_t>& materialIndices);

   // Mesh management
   void SetMesh(MeshHandle mesh);
   [[nodiscard]] const MeshHandle& GetMesh() const noexcept;
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
   // TODO: void SetSubMeshMaterial(size_t index, MaterialHandle material);
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
   std::vector<SubMeshRenderer> m_subMeshRenderers;

   bool m_visible;
   bool m_castsShadows;
   bool m_receivesShadows;
};

