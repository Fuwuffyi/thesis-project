#pragma once

#include <cstddef>

#include "core/GraphicsAPI.hpp"

class Scene;
class ResourceManager;

void LoadBaseScene(Scene& scene, ResourceManager& resourceManager, const GraphicsAPI api,
                   const size_t lightCount = 21, const size_t seed = 42);
