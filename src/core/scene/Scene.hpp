#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

class Node;

class Scene {
public:
   Scene(const std::string& name = "Scene");

   ~Scene();

   Scene(const Scene&) = delete;
   Scene& operator=(const Scene&) = delete;
   Scene(Scene&&) = default;
   Scene& operator=(Scene&&) = default;

   Node* GetRootNode() const { return m_rootNode.get(); }

   // Node management with names for quick lookup
   Node* CreateNode(const std::string& name = "");
   Node* CreateChildNode(Node* parent, const std::string& name = "");
   Node* CreateChildNode(const std::string& parentName, const std::string& childName = "");
   bool AddNode(std::unique_ptr<Node> node, Node* parent = nullptr);
   bool RemoveNode(Node* node);
   bool RemoveNode(const std::string& name);

   // Fast node lookup
   Node* FindNode(const std::string& name) const;
   std::vector<Node*> FindNodes(const std::string& name) const;

   // Scene traversal and operations
   void ForEachNode(const std::function<void(Node*)>& func);
   void ForEachNode(const std::function<void(const Node*)>& func) const;

   // Update systems
   void UpdateTransforms();
   void UpdateScene(const float deltaTime);

   // Scene management
   void Clear();
   size_t GetNodeCount() const;

   // Utility
   const std::string& GetName() const;
   void SetName(const std::string& name);

private:
   void RegisterNode(Node* node);
   void UnregisterNode(Node* node);
   void CollectAllNodes(Node* node, std::vector<Node*>& nodes) const;
   size_t CalculateMaxDepth(const Node* node, size_t currentDepth = 0) const;

private:
   std::string m_name;
   std::unique_ptr<Node> m_rootNode;
   std::unordered_multimap<std::string, Node*> m_nodeRegistry;
   mutable size_t m_nodeCounter;
};

