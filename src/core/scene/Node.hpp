#pragma once

#include <functional>
#include <vector>
#include <memory>

class Transform;
class Component;

class Node {
public:
   explicit Node(const std::string& name = "");
   ~Node();

   Node(const Node&) = delete;
   Node& operator=(const Node&) = delete;
   Node(Node&&) = default;
   Node& operator=(Node&&) = default;

   // Hierarchy management
   void AddChild(const std::unique_ptr<Node> child);
   void AddChild(Node* child);
   bool RemoveChild(const Node* child);
   void RemoveAllChildren();

   // Parent-child relationships
   Node* GetParent() const;
   const std::vector<std::unique_ptr<Node>>& GetChildren() const;
   std::vector<Node*> GetChildrenRaw() const;

   // Tree traversal
   Node* FindChild(const std::string& name, const bool recursive = false) const;
   Node* FindChildByIndex(const size_t index) const;
   size_t GetChildCount() const;
   size_t GetDepth() const;

   // Hierarchy queries
   Node* GetRoot();
   const Node* GetRoot() const;

   // Tree iteration
   void ForEachChild(const std::function<void(Node*)>& func, const bool recursive = false);
   void ForEachChild(const std::function<void(const Node*)>& func, const bool recursive = false) const;

   // Component management
   void AddComponent(std::unique_ptr<Component> component);
   bool RemoveComponent(const Component* component);

   template<typename T>
   T* GetComponent() const {
      const auto typeId = typeid(T).hash_code();
      const auto it = m_componentLookup.find(typeId);
      if (it != m_componentLookup.end()) {
         return static_cast<T*>(it->second);
      }
      return nullptr;
   }

   template<typename T>
   std::vector<T*> GetComponents() const {
      std::vector<T*> result;
      const auto typeId = typeid(T).hash_code();
      for (const auto& component : m_components) {
         if (T* ptr = dynamic_cast<T*>(component.get())) {
            result.push_back(ptr);
         }
      }
      return result;
   }

   template<typename T>
   bool HasComponent() const {
      return GetComponent<T>() != nullptr;
   }

   // Transform access
   Transform* GetTransform() const;
   Transform* GetWorldTransform() const;
   void UpdateWorldTransform(bool force = false);
   void MarkTransformDirty();

   // Utility
   const std::string& GetName() const;
   void SetName(const std::string& name);

   bool IsActive() const;
   void SetActive(const bool active);
private:
   void SetParent(Node* parent);
   void UpdateComponentLookup();
   void InvalidateWorldTransform();
   void UpdateChildrenWorldTransforms();

private:
   // Identity and state
   std::string m_name;
   bool m_active;
   // Hierarchy
   Node* m_parent;
   std::vector<std::unique_ptr<Node>> m_children;
   // Components
   std::vector<std::unique_ptr<Component>> m_components;
   std::unordered_map<size_t, Component*> m_componentLookup;
   // Transform caching
   mutable Transform* m_localTransform;
   mutable std::unique_ptr<Transform> m_worldTransform;
   mutable bool m_worldTransformDirty;
};

