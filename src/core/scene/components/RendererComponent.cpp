#include "core/scene/components/RendererComponent.hpp"

#include "core/resource/IMesh.hpp"

#include <imgui.h>
#include <utility>

RendererComponent::RendererComponent(const MeshHandle mesh, const MaterialHandle material)
    : m_mesh(std::move(mesh)),
      m_material(std::move(material)),
      m_visible(true),
      m_castsShadows(true),
      m_receivesShadows(true) {}

void RendererComponent::DrawInspector(Node* const node) {
   if (ImGui::CollapsingHeader(
          "Renderer", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      ImGui::Text("Material id: %zu", m_material.GetId());
      ImGui::Separator();
      ImGui::Checkbox("Is Visible", &m_visible);
      ImGui::Checkbox("Casts Shadows", &m_castsShadows);
      ImGui::Checkbox("Receives Shadows", &m_receivesShadows);
   }
}

void RendererComponent::SetMesh(const MeshHandle mesh) { m_mesh = std::move(mesh); }

bool RendererComponent::HasMesh() const noexcept { return m_mesh.IsValid(); }

void RendererComponent::SetMaterial(const MaterialHandle material) {
   m_material = std::move(material);
}

bool RendererComponent::HasMaterial() const noexcept { return m_material.IsValid(); }

void RendererComponent::SetVisible(const bool visible) noexcept { m_visible = visible; }

void RendererComponent::SetCastsShadows(const bool castsShadows) noexcept {
   m_castsShadows = castsShadows;
}

void RendererComponent::SetReceivesShadows(const bool receivesShadows) noexcept {
   m_receivesShadows = receivesShadows;
}
