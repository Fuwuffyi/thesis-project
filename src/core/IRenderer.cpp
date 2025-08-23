#include "IRenderer.hpp"

IRenderer::IRenderer(Window* window)
   :
   m_window(window),
   m_activeCamera(nullptr)
{}

void IRenderer::SetActiveCamera(Camera* cam) {
   m_activeCamera = cam;
}

void IRenderer::SetActiveScene(Scene* scene) {
   m_activeScene = scene;
}

ResourceManager* IRenderer::GetResourceManager() {
   return m_resourceManager.get();
}

