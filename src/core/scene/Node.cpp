#include "core/scene/Node.hpp"

#include "core/scene/components/Component.hpp"
#include "core/scene/components/TransformComponent.hpp"

#include <algorithm>

Node::Node(std::string name)
   :
   m_name(std::move(name)),
   m_active(true),
   m_parent(nullptr),
   m_localTransform(nullptr),
   m_worldTransform(nullptr),
   m_worldTransformDirty(true)
{}

Node::~Node() {
   RemoveAllChildren();
   if (m_parent) {
      m_parent->RemoveChild(this);
   }
}

void Node::AddChild(std::unique_ptr<Node> child) {
   if (!child) return;
   // Remove from previous parent if any
   if (child->m_parent) {
      child->m_parent->RemoveChild(child.get());
   }
   child->SetParent(this);
   m_children.emplace_back(std::move(child));
}

bool Node::RemoveChild(const Node* child) {
   if (!child) return false;
   const auto it = std::ranges::find_if(m_children,
                                        [child](const auto& ptr) {
                                        return ptr.get() == child;
                                        });
   if (it != m_children.end()) {
      (*it)->SetParent(nullptr);
      m_children.erase(it);
      return true;
   }
   return false;
}

void Node::RemoveAllChildren() {
   for (const auto& child : m_children) {
      if (child) {
         child->SetParent(nullptr);
      }
   }
   m_children.clear();
}

[[nodiscard]] Node* Node::GetParent() const noexcept {
   return m_parent;
}

[[nodiscard]] const std::vector<std::unique_ptr<Node>>& Node::GetChildren() const noexcept {
   return m_children;
}

[[nodiscard]] std::vector<Node*> Node::GetChildrenRaw() const {
   std::vector<Node*> result;
   result.reserve(m_children.size());
   std::ranges::transform(m_children, std::back_inserter(result),
                          [](const auto& child) { return child.get(); });
   return result;
}

[[nodiscard]] Node* Node::FindChild(const std::string_view& name, const bool recursive) const {
   // Check direct children first
   for (const auto& child : m_children) {
      if (child->GetName() == name) {
         return child.get();
      }
   }
   // Recursive search if requested
   if (recursive) {
      for (const auto& child : m_children) {
         if (Node* found = child->FindChild(name, true)) {
            return found;
         }
      }
   }
   return nullptr;
}

[[nodiscard]] Node* Node::FindChildByIndex(const size_t index) const noexcept {
   return index < m_children.size() ? m_children[index].get() : nullptr;
}

[[nodiscard]] size_t Node::GetChildCount() const noexcept {
   return m_children.size();
}

[[nodiscard]] size_t Node::GetDepth() const noexcept {
   size_t depth = 0;
   const Node* current = this;
   while (current->m_parent) {
      current = current->m_parent;
      ++depth;
   }
   return depth;
}

[[nodiscard]] Node* Node::GetRoot() noexcept {
   Node* current = this;
   while (current->m_parent) {
      current = current->m_parent;
   }
   return current;
}

[[nodiscard]] const Node* Node::GetRoot() const noexcept {
   const Node* current = this;
   while (current->m_parent) {
      current = current->m_parent;
   }
   return current;
}

void Node::ForEachChild(const std::function<void(Node*)>& func, const bool recursive) {
   for (const auto& child : m_children) {
      func(child.get());
      if (recursive) {
         child->ForEachChild(func, true);
      }
   }
}

void Node::ForEachChild(const std::function<void(const Node*)>& func, const bool recursive) const {
   for (const auto& child : m_children) {
      func(child.get());
      if (recursive) {
         const Node* constChild = child.get();
         constChild->ForEachChild(func, true);
      }
   }
}

bool Node::RemoveComponent(const Component* component) {
   if (!component) return false;
   auto it = std::ranges::find_if(m_components,
                                  [component](const auto& ptr) {
                                  return ptr.get() == component;
                                  });
   if (it != m_components.end()) {
      // Clear transform cache if removing transform component
      if (m_localTransform && dynamic_cast<const TransformComponent*>(it->get())) {
         m_localTransform = nullptr;
         MarkTransformDirty();
      }
      m_components.erase(it);
      UpdateComponentLookup();
      return true;
   }
   return false;
}

[[nodiscard]] Transform* Node::GetTransform() const {
   if (!m_localTransform) {
      if (TransformComponent* transformComp = GetComponent<TransformComponent>()) {
         m_localTransform = &transformComp->GetMutableTransform();
      }
   }
   return m_localTransform;
}

[[nodiscard]] Transform* Node::GetWorldTransform() const {
   if (m_worldTransformDirty) {
      const_cast<Node*>(this)->UpdateWorldTransform();
   }
   return m_worldTransform.get();
}

void Node::UpdateWorldTransform(const bool force) {
   if (!m_worldTransformDirty && !force) return;
   Transform* localTransform = GetTransform();
   if (!localTransform) {
      m_worldTransformDirty = false;
      return;
   }
   if (!m_worldTransform) {
      m_worldTransform = std::make_unique<Transform>();
   }
   if (m_parent) {
      if (Transform* parentWorldTransform = m_parent->GetWorldTransform()) {
         // Combine parent world transform with local transform
         const auto parentMatrix = parentWorldTransform->GetTransformMatrix();
         const auto localMatrix = localTransform->GetTransformMatrix();
         const auto worldMatrix = parentMatrix * localMatrix;
         *m_worldTransform = Transform(worldMatrix);
      } else {
         *m_worldTransform = *localTransform;
      }
   } else {
      *m_worldTransform = *localTransform;
   }
   m_worldTransformDirty = false;
   UpdateChildrenWorldTransforms();
}

void Node::MarkTransformDirty() {
   InvalidateWorldTransform();
}

[[nodiscard]] const std::string& Node::GetName() const noexcept {
   return m_name;
}

void Node::SetName(std::string name) {
   m_name = std::move(name);
}

[[nodiscard]] bool Node::IsActive() const noexcept {
   return m_active;
}

void Node::SetActive(const bool active) noexcept {
   if (m_active != active) {
      m_active = active;
   }
}

void Node::SetParent(Node* parent) {
   if (m_parent != parent) {
      m_parent = parent;
      MarkTransformDirty();
   }
}

void Node::UpdateComponentLookup() {
   m_componentLookup.clear();
   for (const auto& component : m_components) {
      const auto typeIndex = std::type_index(typeid(*component));
      m_componentLookup[typeIndex] = component.get();
   }
}

void Node::InvalidateWorldTransform() {
   if (m_worldTransformDirty) return;
   m_worldTransformDirty = true;
   // Propagate to children
   for (const auto& child : m_children) {
      child->InvalidateWorldTransform();
   }
}

void Node::UpdateChildrenWorldTransforms() {
   for (const auto& child : m_children) {
      if (child->m_worldTransformDirty) {
         child->UpdateWorldTransform();
      }
   }
}

