#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

class Node;

class Scene final {
public:
   Scene(std::string name = "Scene");
   ~Scene();

   Scene(const Scene&) = delete;
   Scene& operator=(const Scene&) = delete;
   Scene(Scene&&) = default;
   Scene& operator=(Scene&&) = default;
   
   void DrawInspector();

   [[nodiscard]] Node* GetRootNode() const noexcept;

   // Node management with names for quick lookup
   Node* CreateNode(std::string_view name = {});
   Node* CreateChildNode(Node* parent, std::string_view name = {});
   Node* CreateChildNode(std::string_view parentName, std::string_view childName = {});
   bool AddNode(std::unique_ptr<Node> node, Node* parent = nullptr);
   bool RemoveNode(Node* node);
   bool RemoveNode(std::string_view name);

   // Fast node lookup
   [[nodiscard]] Node* FindNode(std::string_view name) const;
   [[nodiscard]] std::vector<Node*> FindNodes(std::string_view name) const;

   // Scene traversal and operations
   void ForEachNode(const std::function<void(Node*)>& func);
   void ForEachNode(const std::function<void(const Node*)>& func) const;

   // Update systems
   void UpdateTransforms();
   void UpdateScene(const float deltaTime);

   // Scene management
   void Clear();
   [[nodiscard]] size_t GetNodeCount() const noexcept;

   // Utility
   [[nodiscard]] const std::string& GetName() const noexcept;
   void SetName(std::string name);

private:
   void RegisterNode(Node* node);
   void UnregisterNode(Node* node);
   [[nodiscard]] size_t CalculateMaxDepth(const Node* node, const size_t currentDepth = 0) const;

private:
   std::string m_name;
   std::unique_ptr<Node> m_rootNode;
   std::unordered_multimap<std::string, Node*> m_nodeRegistry;
   size_t m_nodeCounter;
};

