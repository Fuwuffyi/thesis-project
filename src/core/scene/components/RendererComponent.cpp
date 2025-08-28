#include "core/scene/components/RendererComponent.hpp"

#include <utility>

RendererComponent::RendererComponent(MeshHandle mesh)
   :
   m_mesh(std::move(mesh)),
   m_visible(true),
   m_castsShadows(true),
   m_receivesShadows(true)
{}

void RendererComponent::SetMesh(MeshHandle mesh) {
   m_mesh = std::move(mesh);
}

[[nodiscard]] const MeshHandle& RendererComponent::GetMesh() const noexcept {
   return m_mesh;
}

[[nodiscard]] bool RendererComponent::HasMesh() const noexcept {
   return m_mesh.IsValid();
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

