#include "RendererComponent.hpp"

RendererComponent::RendererComponent(const MeshHandle& mesh) 
   :
   m_mesh(mesh),
   m_visible(true),
   m_castsShadows(true),
   m_receivesShadows(true)
{}

void RendererComponent::SetMesh(const MeshHandle& mesh) {
   m_mesh = mesh;
}

const MeshHandle& RendererComponent::GetMesh() const {
   return m_mesh;
}

bool RendererComponent::HasMesh() const {
   return m_mesh.IsValid();
}

void RendererComponent::SetVisible(const bool visible) {
   m_visible = visible;
}

bool RendererComponent::IsVisible() const {
   return m_visible;
}

void RendererComponent::SetCastsShadows(const bool castsShadows) {
   m_castsShadows = castsShadows;
}

bool RendererComponent::CastsShadows() const {
   return m_castsShadows;
}

void RendererComponent::SetReceivesShadows(const bool receivesShadows) {
   m_receivesShadows = receivesShadows;
}

bool RendererComponent::ReceivesShadows() const {
   return m_receivesShadows;
}

