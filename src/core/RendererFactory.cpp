#include "RendererFactory.hpp"
#include "Window.hpp"
#include "../gl/GLRenderer.hpp"
#include "../vk/VulkanRenderer.hpp"
#include <stdexcept>

std::unique_ptr<IRenderer> RendererFactory::CreateRenderer(GraphicsAPI api, Window* win) {
    switch (api) {
        case GraphicsAPI::OpenGL:
            return std::make_unique<GLRenderer>(win);
        case GraphicsAPI::Vulkan:
            return std::make_unique<VulkanRenderer>(win);
        default:
            throw std::runtime_error("Unsupported Graphics API");
    }
}
