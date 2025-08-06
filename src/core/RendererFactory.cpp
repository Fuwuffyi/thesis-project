#include "RendererFactory.hpp"
#include "../gl/GLRenderer.hpp"
#include "../vk/VKRenderer.hpp"
#include <stdexcept>

std::unique_ptr<IRenderer> RendererFactory::CreateRenderer(GraphicsAPI api) {
    switch (api) {
        case GraphicsAPI::OpenGL:
            return std::make_unique<GLRenderer>();
        case GraphicsAPI::Vulkan:
            return std::make_unique<VKRenderer>();
        default:
            throw std::runtime_error("Unsupported Graphics API");
    }
}
