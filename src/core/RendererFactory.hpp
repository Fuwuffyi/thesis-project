#pragma once

#include <memory>

#include "GraphicsAPI.hpp"
#include "IRenderer.hpp"

// Forward declaration of the window class
class Window;

class RendererFactory {
  public:
   static std::unique_ptr<IRenderer> CreateRenderer(const GraphicsAPI api, Window* win);
};
