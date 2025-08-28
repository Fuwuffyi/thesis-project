#include "core/scene/Scene.hpp"

#include "core/scene/Node.hpp"

#include "core/scene/components/TransformComponent.hpp"

#include <queue>

Scene::Scene(const std::string& name) : m_name(name) {
   m_rootNode = std::make_unique<Node>("Root");
   m_rootNode->AddComponent(std::make_unique<TransformComponent>());
   RegisterNode(m_rootNode.get());
}

Scene::~Scene() {
   Clear();
}

Node* Scene::CreateNode(const std::string& name) {
   std::string nodeName = name;
   if (nodeName.empty()) {
      nodeName = "Node_" + std::to_string(m_nodeCounter++);
   }
   auto node = std::make_unique<Node>(nodeName);
   node->AddComponent(std::make_unique<TransformComponent>());
   Node* nodePtr = node.get();
   m_rootNode->AddChild(std::move(node));
   RegisterNode(nodePtr);
   return nodePtr;
}

Node* Scene::CreateChildNode(Node* parent, const std::string& name) {
   if (!parent) {
      return CreateNode(name);
   }
   std::string nodeName = name;
   if (nodeName.empty()) {
      nodeName = "Node_" + std::to_string(m_nodeCounter++);
   }
   auto node = std::make_unique<Node>(nodeName);
   node->AddComponent(std::make_unique<TransformComponent>());
   Node* nodePtr = node.get();
   parent->AddChild(std::move(node));
   RegisterNode(nodePtr);
   return nodePtr;
}

Node* Scene::CreateChildNode(const std::string& parentName, const std::string& childName) {
   Node* parent = FindNode(parentName);
   return CreateChildNode(parent, childName);
}

bool Scene::AddNode(std::unique_ptr<Node> node, Node* parent) {
   if (!node) return false;
   Node* nodePtr = node.get();
   if (parent) {
      parent->AddChild(std::move(node));
   } else {
      m_rootNode->AddChild(std::move(node));
   }
   RegisterNode(nodePtr);
   return true;
}

bool Scene::RemoveNode(Node* node) {
   if (!node || node == m_rootNode.get()) {
      return false;
   }
   UnregisterNode(node);
   Node* parent = node->GetParent();
   if (parent) {
      return parent->RemoveChild(node);
   }
   return false;
}

bool Scene::RemoveNode(const std::string& name) {
   Node* node = FindNode(name);
   if (node) {
      return RemoveNode(node);
   }
   return false;
}

Node* Scene::FindNode(const std::string& name) const {
   auto it = m_nodeRegistry.find(name);
   if (it != m_nodeRegistry.end()) {
      return it->second;
   }
   return nullptr;
}

std::vector<Node*> Scene::FindNodes(const std::string& name) const {
   std::vector<Node*> result;
   auto range = m_nodeRegistry.equal_range(name);
   for (auto it = range.first; it != range.second; ++it) {
      result.push_back(it->second);
   }
   return result;
}

void Scene::ForEachNode(const std::function<void(Node*)>& func) {
   if (!func || !m_rootNode) return;
   std::queue<Node*> queue;
   queue.push(m_rootNode.get());
   while (!queue.empty()) {
      Node* current = queue.front();
      queue.pop();
      func(current);
      // Add children to queue
      for (Node* child : current->GetChildrenRaw()) {
         queue.push(child);
      }
   }
}

void Scene::ForEachNode(const std::function<void(const Node*)>& func) const {
   if (!func || !m_rootNode) return;
   std::queue<const Node*> queue;
   queue.push(m_rootNode.get());
   while (!queue.empty()) {
      const Node* current = queue.front();
      queue.pop();
      func(current);
      for (const auto& child : current->GetChildren()) {
         queue.push(child.get());
      }
   }
}

void Scene::UpdateTransforms() {
   if (m_rootNode) {
      m_rootNode->UpdateWorldTransform(true);
   }
}

void Scene::UpdateScene(const float deltaTime) {
   UpdateTransforms();
   // TODO: Add component updates
   // ForEachNode([deltaTime](Node* node) {
   //     // Update components that need per-frame updates
   // });
}

void Scene::Clear() {
   m_nodeRegistry.clear();
   if (m_rootNode) {
      m_rootNode->RemoveAllChildren();
   }
   m_nodeCounter = 0;
}

size_t Scene::GetNodeCount() const {
   return m_nodeRegistry.size();
}

const std::string& Scene::GetName() const {
   return m_name;
}

void Scene::SetName(const std::string& name) {
   m_name = name;
}

void Scene::RegisterNode(Node* node) {
   if (node) {
      m_nodeRegistry.emplace(node->GetName(), node);
   }
}

void Scene::UnregisterNode(Node* node) {
   if (!node) return;
   // Remove all entries with this node pointer
   const auto range = m_nodeRegistry.equal_range(node->GetName());
   for (auto it = range.first; it != range.second;) {
      if (it->second == node) {
         it = m_nodeRegistry.erase(it);
      } else {
         ++it;
      }
   }
   // Also unregister all children
   for (Node* child : node->GetChildrenRaw()) {
      UnregisterNode(child);
   }
}

void Scene::CollectAllNodes(Node* node, std::vector<Node*>& nodes) const {
   if (!node) return;
   nodes.push_back(node);
   for (Node* child : node->GetChildrenRaw()) {
      CollectAllNodes(child, nodes);
   }
}

size_t Scene::CalculateMaxDepth(const Node* node, const size_t currentDepth) const {
   if (!node) return currentDepth;
   size_t maxDepth = currentDepth;
   for (const auto& child : node->GetChildren()) {
      const size_t childDepth = CalculateMaxDepth(child.get(), currentDepth + 1);
      maxDepth = std::max(maxDepth, childDepth);
   }
   return maxDepth;
}

