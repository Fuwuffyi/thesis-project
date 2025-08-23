#pragma once

#include "core/Vertex.hpp"

#include <string>
#include <utility>
#include <vector>

namespace MeshLoader {
   std::pair<std::vector<Vertex>, std::vector<uint16_t>> GetMeshData(const std::string& filepath);
};
