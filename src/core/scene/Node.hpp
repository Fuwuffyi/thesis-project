#pragma once

#include "core/scene/components/TransformComponent.hpp"

#include <functional>
#include <typeindex>
#include <memory>
#include <ranges>

class Transform;
class Component;

class Node final {
  public:
   explicit Node(const std::string name = "");
   ~Node();

   Node(const Node&) = delete;
   Node& operator=(const Node&) = delete;
   Node(Node&&) = default;
   Node& operator=(Node&&) = default;

   // Hierarchy management
   void AddChild(std::unique_ptr<Node> child);
   [[nodiscard]] bool RemoveChild(const Node* child);
   void RemoveAllChildren();

   // Parent-child relationships
   [[nodiscard]] constexpr Node* GetParent() const noexcept { return m_parent; }
   [[nodiscard]] constexpr const std::vector<std::unique_ptr<Node>>& GetChildren() const noexcept {
      return m_children;
   }
   [[nodiscard]] std::vector<Node*> GetChildrenRaw() const;

   // Tree traversal
   [[nodiscard]] Node* FindChild(const std::string_view name, const bool recursive = false) const;
   [[nodiscard]] Node* FindChildByIndex(const size_t index) const noexcept;
   [[nodiscard]] constexpr size_t GetChildCount() const noexcept { return m_children.size(); }
   [[nodiscard]] size_t GetDepth() const noexcept;

   // Hierarchy queries
   [[nodiscard]] Node* GetRoot() noexcept;
   [[nodiscard]] const Node* GetRoot() const noexcept;

   // Tree iteration
   void ForEachChild(const std::function<void(Node*)>& func, bool recursive = false);
   void ForEachChild(const std::function<void(const Node*)>& func, bool recursive = false) const;

   // Component management
   template <ComponentType T, typename... Args>
   [[nodiscard]] T* AddComponent(Args&&... args) {
      auto component = std::make_unique<T>(std::forward<Args>(args)...);
      T* const ptr = component.get();
      m_components.emplace_back(std::move(component));
      UpdateComponentLookup();
      // Cache transform component for quick access
      if constexpr (std::same_as<T, TransformComponent>) {
         m_localTransform = &ptr->GetMutableTransform();
         MarkTransformDirty();
      }
      return ptr;
   }

   [[nodiscard]] bool RemoveComponent(const Component* component);

   template <ComponentType T>
   [[nodiscard]] T* GetComponent() const noexcept {
      const std::type_index typeIndex(typeid(T));
      if (const auto it = m_componentLookup.find(typeIndex); it != m_componentLookup.end()) {
         return static_cast<T*>(it->second);
      }
      return nullptr;
   }

   template <ComponentType T>
   [[nodiscard]] std::vector<T*> GetComponents() const {
      std::vector<T*> result;
      const auto componentView =
         m_components | std::ranges::views::transform([](const auto& comp) { return comp.get(); }) |
         std::ranges::views::filter(
            [](const Component* const comp) { return dynamic_cast<const T*>(comp) != nullptr; }) |
         std::ranges::views::transform([](Component* const comp) { return static_cast<T*>(comp); });
      std::ranges::copy(componentView, std::back_inserter(result));
      return result;
   }

   template <ComponentType T>
   [[nodiscard]] bool HasComponent() const noexcept {
      return GetComponent<T>() != nullptr;
   }

   // Transform access
   [[nodiscard]] Transform* GetTransform() const;
   [[nodiscard]] Transform* GetWorldTransform() const;
   void UpdateWorldTransform(const bool force = false);
   void MarkTransformDirty();

   // Utility
   [[nodiscard]] constexpr const std::string& GetName() const noexcept { return m_name; }
   void SetName(const std::string name);

   [[nodiscard]] constexpr bool IsActive() const noexcept { return m_active; }
   void SetActive(const bool active) noexcept;

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
   std::unordered_map<std::type_index, Component*> m_componentLookup;
   // Transform caching
   mutable Transform* m_localTransform;
   mutable std::unique_ptr<Transform> m_worldTransform;
   mutable bool m_worldTransformDirty;
};
