#include "core/IRenderer.hpp"

IRenderer::IRenderer(Window* window) noexcept
    : m_window(window), m_activeCamera(nullptr), m_activeScene(nullptr) {}

void IRenderer::SetActiveCamera(Camera* cam) noexcept { m_activeCamera = cam; }

void IRenderer::SetActiveScene(Scene* scene) noexcept { m_activeScene = scene; }
