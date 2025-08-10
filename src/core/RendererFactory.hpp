#pragma once

#include "IRenderer.hpp"
#include "GraphicsAPI.hpp"
#include <memory>

// Forward declaration of the window class
class Window;

class RendererFactory {
public:
    static std::unique_ptr<IRenderer> CreateRenderer(GraphicsAPI api, Window* win);
};
