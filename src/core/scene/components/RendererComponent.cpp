#include "core/scene/components/RendererComponent.hpp"

#include "core/resource/IMesh.hpp"

#include <imgui.h>
#include <utility>

RendererComponent::RendererComponent(MeshHandle mesh)
   :
   m_mesh(std::move(mesh)),
   m_visible(true),
   m_castsShadows(true),
   m_receivesShadows(true)
{}

RendererComponent::RendererComponent(const std::vector<MeshHandle>& meshes,
                                     const std::vector<uint32_t>& materialIndices) {
   m_subMeshRenderers.reserve(meshes.size());
   for (size_t i = 0; i < meshes.size() && i < materialIndices.size(); ++i) {
      SubMeshRenderer renderer;
      renderer.mesh = meshes[i];
      renderer.materialIndex = materialIndices[i];
      m_subMeshRenderers.push_back(std::move(renderer));
   }
}

void RendererComponent::DrawInspector(Node* node) {
   if (ImGui::CollapsingHeader("Renderer",
                               ImGuiTreeNodeFlags_DefaultOpen |
                               ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      ImGui::Checkbox("Is Visible", &m_visible);
      ImGui::Checkbox("Casts Shadows", &m_castsShadows);
      ImGui::Checkbox("Receives Shadows", &m_receivesShadows);
   }
}

void RendererComponent::SetMesh(MeshHandle mesh) {
   m_mesh = std::move(mesh);
   m_subMeshRenderers.clear();
}

[[nodiscard]] const MeshHandle& RendererComponent::GetMesh() const noexcept {
   return m_mesh;
}

[[nodiscard]] bool RendererComponent::HasMesh() const noexcept {
   return m_mesh.IsValid() || !m_subMeshRenderers.empty();
}

void RendererComponent::SetSubMeshRenderers(const std::vector<SubMeshRenderer>& renderers) {
   m_subMeshRenderers = renderers;
   m_mesh = MeshHandle{};
}

void RendererComponent::AddSubMeshRenderer(const SubMeshRenderer& renderer) {
   m_subMeshRenderers.push_back(renderer);
}

void RendererComponent::RemoveSubMeshRenderer(const size_t index) {
   if (index < m_subMeshRenderers.size()) {
      m_subMeshRenderers.erase(m_subMeshRenderers.begin() + index);
   }
}

const std::vector<RendererComponent::SubMeshRenderer>& RendererComponent::GetSubMeshRenderers() const noexcept {
   return m_subMeshRenderers;
}

std::vector<RendererComponent::SubMeshRenderer>& RendererComponent::GetSubMeshRenderers() noexcept {
   return m_subMeshRenderers;
}

size_t RendererComponent::GetSubMeshCount() const noexcept {
   return m_subMeshRenderers.size();
}

void RendererComponent::SetSubMeshVisible(const size_t index, const bool visible) {
   if (index < m_subMeshRenderers.size()) {
      m_subMeshRenderers[index].visible = visible;
   }
}

bool RendererComponent::IsSubMeshVisible(const size_t index) const {
   return index < m_subMeshRenderers.size() ? m_subMeshRenderers[index].visible : false;
}

void RendererComponent::SetVisible(const bool visible) noexcept {
   m_visible = visible;
}

[[nodiscard]] bool RendererComponent::IsVisible() const noexcept {
   return m_visible;
}

void RendererComponent::SetCastsShadows(const bool castsShadows) noexcept {
   m_castsShadows = castsShadows;
}

[[nodiscard]] bool RendererComponent::CastsShadows() const noexcept {
   return m_castsShadows;
}

void RendererComponent::SetReceivesShadows(const bool receivesShadows) noexcept {
   m_receivesShadows = receivesShadows;
}

[[nodiscard]] bool RendererComponent::ReceivesShadows() const noexcept {
   return m_receivesShadows;
}

bool RendererComponent::IsMultiMesh() const noexcept {
   return !m_subMeshRenderers.empty();
}

void RendererComponent::ClearSubMeshes() {
   m_subMeshRenderers.clear();
}

