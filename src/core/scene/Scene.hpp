#pragma once

#include <vector>
#include <memory>

class Node;

class Scene {
public:
   Scene();
   ~Scene();

   Node* GetRootNode() const;

private:
   std::unique_ptr<Node> m_rootNode;
};

