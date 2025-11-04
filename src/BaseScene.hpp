#pragma once

#include "core/GraphicsAPI.hpp"

class Scene;
class ResourceManager;

void LoadBaseScene(Scene& scene, ResourceManager& resourceManager, const GraphicsAPI api,
                   const std::size_t lightCount = 21, const std::size_t seed = 42);
