#include "IRenderer.hpp"

IRenderer::IRenderer(Window* window)
   :
   m_window(window),
   m_activeCamera(nullptr)
{}

void IRenderer::SetActiveCamera(Camera* cam) {
   m_activeCamera = cam;
}

