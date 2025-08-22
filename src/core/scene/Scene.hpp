#pragma once

#include <vector>

class Node;

class Scene {
public:
   ~Scene();

private:
   std::vector<Node> m_nodes;
};

