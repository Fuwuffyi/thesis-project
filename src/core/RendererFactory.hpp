#pragma once

#include "IRenderer.hpp"
#include "GraphicsAPI.hpp"
#include <memory>

class RendererFactory {
public:
    static std::unique_ptr<IRenderer> CreateRenderer(GraphicsAPI api);
};
