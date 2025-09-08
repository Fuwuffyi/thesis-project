#pragma once

#include <memory>

#include "core/GraphicsAPI.hpp"
#include "core/IRenderer.hpp"

class Window;

class RendererFactory final {
  public:
   [[nodiscard]] static std::unique_ptr<IRenderer> CreateRenderer(const GraphicsAPI api, Window* const win);
};
