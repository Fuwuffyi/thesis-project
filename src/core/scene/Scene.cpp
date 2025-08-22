#include "Scene.hpp"

#include "Node.hpp"
#include "components/TransformComponent.hpp"

Scene::Scene()
:
   m_rootNode(std::make_unique<Node>())
{
   // Set world transform as default transform
   m_rootNode->AddComponent(std::make_unique<TransformComponent>());
}

Scene::~Scene() = default;

Node* Scene::GetRootNode() const {
   return m_rootNode.get();
}

