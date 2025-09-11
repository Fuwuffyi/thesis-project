#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string_view>

class Node;
class MaterialEditor;

class Scene final {
  public:
   explicit Scene(const std::string name = "Scene");
   ~Scene();

   Scene(const Scene&) = delete;
   Scene& operator=(const Scene&) = delete;
   Scene(Scene&&) = default;
   Scene& operator=(Scene&&) = default;

   void DrawInspector(MaterialEditor& matEditor);

   [[nodiscard]] constexpr Node* GetRootNode() const noexcept { return m_rootNode.get(); }

   // Node management with names for quick lookup
   [[nodiscard]] Node* CreateNode(const std::string_view name = {});
   [[nodiscard]] Node* CreateChildNode(Node* parent, const std::string_view name = {});
   [[nodiscard]] Node* CreateChildNode(const std::string_view parentName,
                                       const std::string_view childName = {});
   [[nodiscard]] bool AddNode(const std::unique_ptr<Node> node, Node* parent = nullptr);
   [[nodiscard]] bool RemoveNode(const Node* node);
   [[nodiscard]] bool RemoveNode(const std::string_view name);

   // Node lookup
   [[nodiscard]] Node* FindNode(const std::string_view name) const;
   [[nodiscard]] std::vector<Node*> FindNodes(const std::string_view name) const;

   // Scene traversal and operations
   void ForEachNode(const std::function<void(Node*)>& func);
   void ForEachNode(const std::function<void(const Node*)>& func) const;

   // Update systems
   void UpdateTransforms();
   void UpdateScene(const float deltaTime);

   // Scene management
   void Clear();
   [[nodiscard]] constexpr size_t GetNodeCount() const noexcept { return m_nodeRegistry.size(); }

   // Utility
   [[nodiscard]] constexpr const std::string& GetName() const noexcept { return m_name; }
   void SetName(const std::string name);

  private:
   void RegisterNode(const Node* node);
   void UnregisterNode(const Node* node);
   [[nodiscard]] size_t CalculateMaxDepth(const Node* node, const size_t currentDepth = 0) const;

  private:
   std::string m_name;
   std::unique_ptr<Node> m_rootNode;
   std::unordered_multimap<std::string, Node*> m_nodeRegistry;
   size_t m_nodeCounter;
};
