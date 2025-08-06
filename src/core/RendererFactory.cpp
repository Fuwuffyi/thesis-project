#include "RendererFactory.hpp"
#include "Window.hpp"
#include "../gl/GLRenderer.hpp"
#include "../vk/VKRenderer.hpp"
#include <stdexcept>

std::unique_ptr<IRenderer> RendererFactory::CreateRenderer(GraphicsAPI api, const Window& win) {
    switch (api) {
        case GraphicsAPI::OpenGL:
            return std::make_unique<GLRenderer>(win.GetNativeWindow());
        case GraphicsAPI::Vulkan:
            return std::make_unique<VKRenderer>(win.GetNativeWindow());
        default:
            throw std::runtime_error("Unsupported Graphics API");
    }
}
